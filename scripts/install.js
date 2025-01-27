import fs from 'fs';
import { chmod as chmodSync } from 'fs-chmod';
import fetch from 'node-fetch';
import semver from 'semver';
import * as unzipper from 'unzipper';
import { pipeline } from 'stream/promises';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';
import * as dotenv from 'dotenv';

dotenv.config();

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// Read package.json for version
const pkg = JSON.parse(
  fs.readFileSync(join(__dirname, '..', 'package.json'), 'utf8')
);

const supportedPlatforms = ["darwin", "linux", "win32"];

function getBinaryUrl() {
  const endpoint = "https://github.com/justxd22/qml-parser/releases/download";
  const version = semver.coerce(pkg.version);
  const { platform } = process;

  if (!supportedPlatforms.includes(platform)) {
    throw new Error(`Unsupported ${platform} platform.`);
  }

  return platform === "win32"
    ? `${endpoint}/v${version}/windows.zip`
    : `${endpoint}/v${version}/${platform}.zip`;
}

async function main() {
  if (process.env.QML_PARSER_DISABLE_DOWNLOAD) {
    console.log(
      '[INFO] Skipping binary download. "QML_PARSER_DISABLE_DOWNLOAD" environment variable was found.'
    );
    return;
  }

  const binaryUrl = getBinaryUrl();
  const installPath = `${__dirname}/../vendor`;

  console.log("[INFO] Downloading binary from", binaryUrl);

  if (fs.existsSync(installPath)) {
    console.log("[INFO] Removing existing installation...");
    fs.rmSync(installPath, { recursive: true });
  }
  fs.mkdirSync(installPath);
  console.log("[INFO] Downloading and extracting...");

  const res = await fetch(binaryUrl);

  if (!res.ok) {
    throw new Error("Unable to fetch binaries");
  }

  const totalSize = res.headers.get("content-length");
  let downloaded = 0;

  res.body.on("data", (chunk) => {
    downloaded += chunk.length;
    const percentage = ((downloaded / totalSize) * 100).toFixed(2);
    process.stdout.write(`\r[INFO] Downloading... ${percentage}%`);
  });

  await pipeline(res.body, unzipper.Extract({ path: installPath }));

  if (process.platform === "darwin") {
    fs.symlinkSync(
      `qml-parser.app/Contents/MacOS/qml-parser`,
      `${installPath}/qml-parser`
    );
  }
  if (process.platform !== "win32") {
    chmodSync(`${installPath}/qml-parser`, "+x");
  }
}

main();
