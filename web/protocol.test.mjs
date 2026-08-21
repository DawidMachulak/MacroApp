// Testy jednostkowe warstwy protokołu (bez DOM, bez navigator.serial).
// Uruchomienie:  node --test web/protocol.test.mjs
import { test } from "node:test";
import assert from "node:assert/strict";
import { createRequire } from "node:module";

const require = createRequire(import.meta.url);
const {
  ramkaPing, ramkaGetConfig, ramkaSetLayer, ramkaSave, ramkaSetActive,
  podzielNaLinie, zlozKonfigZWarstw,
} = require("./protocol.js");

test("budowniczowie ramek zwracają kształt zgodny z docs/06 §4", () => {
  assert.deepEqual(ramkaPing(), { cmd: "ping" });
  assert.deepEqual(ramkaGetConfig(), { cmd: "get_config" });
  assert.deepEqual(ramkaSetLayer(2, { n: "X" }), { cmd: "set_layer", idx: 2, data: { n: "X" } });
  assert.deepEqual(ramkaSave(), { cmd: "save" });
  assert.deepEqual(ramkaSetActive(1), { cmd: "set_active", idx: 1 });
});

test("podzielNaLinie: pojedyncza kompletna linia", () => {
  const { linie, bufor } = podzielNaLinie("", '{"cmd":"pong"}\n');
  assert.deepEqual(linie, ['{"cmd":"pong"}']);
  assert.equal(bufor, "");
});

test("podzielNaLinie: kilka ramek w jednym kawałku naraz", () => {
  const { linie, bufor } = podzielNaLinie("", '{"a":1}\n{"a":2}\n{"a":3}\n');
  assert.deepEqual(linie, ['{"a":1}', '{"a":2}', '{"a":3}']);
  assert.equal(bufor, "");
});

test("podzielNaLinie: jedna ramka rozbita na dwa kawałki (bufor łączy)", () => {
  const krok1 = podzielNaLinie("", '{"cmd":"lay');
  assert.deepEqual(krok1.linie, []);
  assert.equal(krok1.bufor, '{"cmd":"lay');

  const krok2 = podzielNaLinie(krok1.bufor, 'er","idx":0}\n');
  assert.deepEqual(krok2.linie, ['{"cmd":"layer","idx":0}']);
  assert.equal(krok2.bufor, "");
});

test("podzielNaLinie: puste linie (same \\n) są odfiltrowywane", () => {
  const { linie, bufor } = podzielNaLinie("", '\n\n{"a":1}\n\n');
  assert.deepEqual(linie, ['{"a":1}']);
  assert.equal(bufor, "");
});

test("zlozKonfigZWarstw: składa warstwy niezależnie od kolejności odbioru", () => {
  const fragmenty = [
    { idx: 1, data: { n: "IDE" } },
    { idx: 0, data: { n: "FPS" } },
  ];
  const cfg = zlozKonfigZWarstw(fragmenty, 1, 0);
  assert.equal(cfg.v, 1);
  assert.equal(cfg.active, 0);
  assert.deepEqual(cfg.ly, [{ n: "FPS" }, { n: "IDE" }]);
});

test("zlozKonfigZWarstw: brakująca warstwa w środku sekwencji rzuca błąd", () => {
  const fragmenty = [{ idx: 0, data: { n: "FPS" } }, { idx: 2, data: { n: "X" } }]; // brak idx 1
  assert.throws(() => zlozKonfigZWarstw(fragmenty, 1, 0), /brakuje/);
});

test("zlozKonfigZWarstw: pusta lista rzuca błąd zamiast zwracać pusty config", () => {
  assert.throws(() => zlozKonfigZWarstw([], 1, 0), /nie zwróciło żadnej warstwy/);
});

test("zlozKonfigZWarstw: brak v/active w argumentach -> sensowne wartości domyślne", () => {
  const cfg = zlozKonfigZWarstw([{ idx: 0, data: { n: "X" } }], undefined, undefined);
  assert.equal(cfg.v, 1);
  assert.equal(cfg.active, 0);
});
