import fs from 'fs';
import { chmod as chmodSync } from 'fs-chmod';
import fetch from 'node-fetch';
import semver from 'semver';
import * as unzipper from 'unzipper';
import { pipeline } from 'stream/promises';
import { fileURLToPath } from 'url';
import { dirname, join, resolve } from 'path';
import * as dotenv from 'dotenv';
import { cpSync } from 'fs';

dotenv.config();

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// Read package.json for version
const pkg = JSON.parse(
  fs.readFileSync(join(__dirname, '..', 'package.json'), 'utf8')
);

const supportedPlatforms = ["darwin", "linux", "win32"];
const distPath = resolve(__dirname, '../dist');
const installPath = resolve(__dirname, '../vendor');

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

async function copyFromDist() {
  console.log("[INFO] Copying binaries from dist folder...");
  if (fs.existsSync(installPath)) {
    fs.rmSync(installPath, { recursive: true });
  }
  fs.mkdirSync(installPath, { recursive: true });
  cpSync(distPath, installPath, { recursive: true });
  console.log("[INFO] Binaries copied successfully.");
}

async function downloadAndExtract() {
  const binaryUrl = getBinaryUrl();
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

async function main() {
  if (fs.existsSync(distPath) && fs.readdirSync(distPath).length > 0) {
    await copyFromDist();
  } else {
    await downloadAndExtract();
  }
}

main();
