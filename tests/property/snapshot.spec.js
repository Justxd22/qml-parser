import fs from "fs";
import { parse } from "../../src/index.js";

import { fileURLToPath } from "url";
import { dirname } from "path";

// Get __dirname equivalent in ES Modules
const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

it("renders correctly", () => {
  const ast = parse(fs.readFileSync(`${__dirname}/entry.qml`).toString());

  expect(ast).toMatchSnapshot();
});
