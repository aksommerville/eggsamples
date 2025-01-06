/* genindex.js
 * Given a directory containing Egg games in HTML format, generate "index.html" in that same directory.
 * If index.html already exists, quietly clobber it.
 * It would be easier to do this against the project's source "metadata" file,
 * but I choose to do it the hard way, against the finished HTML.
 * That means you can use this script against any collection of finished Egg games.
 */
 
const fs = require("fs");

if (process.argv.length !== 3) {
  throw new Error(`Usage: ${process.argv[1]} DIRECTORY`);
}
const DIR = process.argv[2];

/* (src) is a base64-encoded ROM exactly as stored in the HTML.
 * Returns array of { tid, rid, v } where (v) is a Buffer.
 */
function resourcesFromBase64Rom(src) {
  const rom = Buffer.from(src, "base64");
  if (
    (rom.length < 4) ||
    (rom[0] !== 0x00) ||
    (rom[1] !== 0x45) ||
    (rom[2] !== 0x47) ||
    (rom[3] !== 0x47)
  ) throw new Error(`Egg ROM signature mismatch`);
  const resv = []; // {tid,rid,p,c}
  for (let romp=4, tid=1, rid=1; romp<rom.length; ) {
    const lead = rom[romp++];
    if (!lead) break; // EOF
    switch (lead & 0xc0) {
      case 0x00: { // TID
          tid += lead;
          rid = 1;
        } break;
      case 0x40: { // RID
          let d = ((lead & 0x3f) << 8) | rom[romp++];
          if (!d) throw new Error(`Malformed ROM (RID delta zero)`);
          rid += d;
        } break;
      case 0x80: { // SMALL
          let len = (lead & 0x3f) << 8;
          len |= rom[romp++];
          len += 1;
          if ((tid > 0xff) || (rid > 0xffff) || (romp > rom.length - len)) {
            throw new Error(`Malformed ROM (tid=${tid} rid=${rid} len=${len} at ${romp}/${rom.length})`);
          }
          resv.push({ tid, rid, v: rom.slice(romp, romp + len) });
          rid++;
          romp += len;
        } break;
      case 0xc0: { // LARGE
          let len = (lead & 0x3f) << 16;
          len |= rom[romp++] << 8;
          len |= rom[romp++];
          len += 16385;
          if ((tid > 0xff) || (rid > 0xffff) || (romp > rom.length - len)) {
            throw new Error(`Malformed ROM (tid=${tid} rid=${rid} len=${len} at ${romp}/${rom.length})`);
          }
          resv.push({ tid, rid, v: rom.slice(romp, romp + len) });
          rid++;
          romp += len;
        } break;
    }
  }
  return resv;
}

/* Return array of [key, value] for a metadata resource.
 */
function readMetadata(src) {
  if (
    (src.length < 4) ||
    (src[0] !== 0x00) ||
    (src[1] !== 0x45) ||
    (src[2] !== 0x4d) ||
    (src[3] !== 0xff)
  ) throw new Error(`metadata signature mismatch`);
  const dst = [];
  for (let srcp=4; srcp<src.length; ) {
    const kc = src[srcp++] || 0;
    if (!kc) break; // Zero-length key is an explicit terminator.
    const vc = src[srcp++] || 0;
    if (srcp > src.length - vc - kc) {
      throw new Error(`metadata field overrun`);
    }
    const k = src.toString("utf8", srcp, srcp + kc); srcp += kc;
    const v = src.toString("utf8", srcp, srcp + vc); srcp += vc;
    dst.push([k, v]);
  }
  return dst;
}

/* Given a partial Game object and raw text from the <egg-rom> tag,
 * decode the ROM, find the metadata, and populate (game).
 */
function decodeGame(game, src) {
  const resv = resourcesFromBase64Rom(src);
  for (const { tid, rid, v } of resv) {
    switch (tid) {
      case 1: { // metadata -- what we're mostly interested in
          if (rid !== 1) throw new Error(`Illegal metadata rid ${rid}`);
          for (const [key, value] of readMetadata(v)) {
            switch (key) {
              case "title": game.title = value; break;
              case "author": game.author = value; break;
              case "time": game.time = value; break;
              case "version": game.version = value; break;
              case "players": game.players = value; break;
              case "desc": game.desc = value; break;
              case "genre": game.genre = value; break;
              case "tags": game.tags = value; break;
            }
          }
        } break;
      // Could also use 3=strings and 4=image.
      // For now at least, we're only using the default text from metadata, and we yoink the icon from its <link> tag.
    }
  }
}

function iconFromLinkTag(src) {
  let rel="", href="";
  for (const field of src.split(/\s+/g)) {
    if (field.startsWith("rel=")) rel = field.substring(4);
    else if (field.startsWith("href=")) href = field.substring(5);
  }
  if ((rel !== '"icon"') || !href.startsWith('"data:')) return "";
  return href.substring(1, href.length - 1);
}

function readGameFromHtmlPath(path) {
  const html = fs.readFileSync(path).toString("utf8");
  if (!html.startsWith("<!DOCTYPE html>")) throw new Error(`No HTML signature`);
  
  /* Read tags naively.
   * We are interested in <link rel="icon"...> and <egg-rom>...</egg-rom>.
   * The <egg-rom> tag won't have any nested tags.
   */
  let icon = "";
  let rom = "";
  for (let htmlp=1; htmlp<html.length; ) {
    let head="", body=""; // head is the tag name and attributes
    const openp = html.indexOf("<", htmlp);
    if (openp < 0) break;
    const closep = html.indexOf(">", openp);
    if (closep < 0) break;
    htmlp = closep + 1;
    head = html.substring(openp + 1, closep);
    const tagName = head.match(/^\s*([a-zA-Z0-9_-]+)/)?.[1] || "";
    if (head.endsWith("/")) {
      head = head.substring(0, head.length - 1);
    } else if (tagName === "script") {
      const nextp = html.indexOf("</script>", closep);
      if (nextp < 0) break;
      body = html.substring(closep + 1, nextp);
      htmlp = nextp + 1;
    } else {
      const nextp = html.indexOf("<", closep);
      if (nextp < 0) break;
      if (html.substring(nextp, nextp + 2 + tagName.length) === "</" + tagName) {
        body = html.substring(closep + 1, nextp);
        htmlp = nextp + 1;
      }
    }
    switch (tagName) {
      case "link": if (!icon) icon = iconFromLinkTag(head); break;
      case "egg-rom": rom = body; break;
    }
  }
  if (!rom) throw new Error(`No ROM`);
  
  const game = { path, icon };
  decodeGame(game, rom);
  return game;
}

/* Acquire the games.
 */
const games = [];
for (const base of fs.readdirSync(DIR)) {

  if (base === "index.html") continue;
  const ext = base.match(/\.([^.]*)$/)?.[1].toLowerCase();
  if ((ext !== "html") && (ext !== "htm")) {
    console.log(`${DIR}/${base}: Ignoring file due to unexpected extension`);
    continue;
  }
  
  try {
    const game = readGameFromHtmlPath(DIR + "/" + base);
    if (!game) throw null;
    game.path = "./" + base; // Replace fs path with the one we'll link to.
    if (!game.title) game.title = base;
    games.push(game);
  } catch (e) {
    console.log(`${DIR}/${base}: Failed to read Egg HTML game.`);
    console.log(e);
  }
}
games.sort((a, b) => {
  const at = a.title.toUpperCase();
  const bt = b.title.toUpperCase();
  if (at < bt) return -1;
  if (at > bt) return 1;
  return 0;
});

/* Generate HTML.
 */
let html = "<!DOCTYPE html>\n<head>\n";
//TODO Stylesheet.
//TODO Icon?
//TODO Script?
html += "</head><body>\n";
html += "<ul class=\"games\">\n";
for (const game of games) {
  html += "<li>";
  if (game.icon) {
    html += `<img class="icon" src="${game.icon}" />\n`;
  }
  html += `<a class="launch" href="${game.path}">${game.title}</a>\n`;
  //TODO We have other metadata: author, time, genre, tags, players, version, desc
  html += "</li>";
}
html += "</ul>\n";
html += "</body></html>\n";

fs.writeFileSync(DIR + "/index.html", html);
