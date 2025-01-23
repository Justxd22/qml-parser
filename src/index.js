import { execSync } from 'child_process';
import { platform } from 'os';
import { join } from 'path';
import { fileURLToPath } from 'url';
import { dirname } from 'path';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

function getBinPath() {
  switch (platform()) {
    case "darwin":
      return join(__dirname, "../vendor/qml-parser.app/Contents/MacOS/qml-parser");
    case "linux":
      return join(__dirname, "../vendor/qml-parser");
    case "win32":
      return join(__dirname, "../vendor/qml-parser.exe");
    default:
      throw new Error(`Unsupported ${platform()} platform.`);
  }
}

function parse(code) {
  const bin = getBinPath();
  const result = execSync(bin, { 
    input: code,
    maxBuffer: Infinity 
  });
  const ast = JSON.parse(result);
  return ast || {};
}

function parseFile(filepath) {
  const bin = getBinPath();
  const result = execSync(`${bin} ${filepath}`, { maxBuffer: Infinity });
  const ast = JSON.parse(result);
  return ast || {};
}

export {
  parse,
  parseFile,
};