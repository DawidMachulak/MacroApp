/*
 * Warstwa protokołu Web Serial (Krok 2) — docs/06-architektura-i-json.md §4.
 *
 * Celowo bez zależności od DOM/navigator.serial, żeby dało się to
 * przetestować samym Node (patrz protocol.test.mjs) i wczytać zarówno
 * w przeglądarce (<script src="protocol.js">), jak i przez require()/import.
 */
(function (root) {
  "use strict";

  function ramkaPing() { return { cmd: "ping" }; }
  function ramkaGetConfig() { return { cmd: "get_config" }; }
  function ramkaSetLayer(idx, data) { return { cmd: "set_layer", idx, data }; }
  function ramkaSave() { return { cmd: "save" }; }
  function ramkaSetActive(idx) { return { cmd: "set_active", idx }; }

  // Dzieli strumień tekstu na kompletne linie (ramki kończą się \n).
  // `bufor` to niedokończona reszta z poprzedniego wywołania, `kawalek` to
  // nowo odebrany fragment. Zwraca gotowe do JSON.parse linie oraz nowy
  // bufor do przekazania przy kolejnym wywołaniu — działa niezależnie od
  // tego, czy jedna ramka przyjdzie w kilku kawałkach, czy kilka ramek
  // w jednym kawałku.
  function podzielNaLinie(bufor, kawalek) {
    const polaczone = bufor + kawalek;
    const czesci = polaczone.split("\n");
    const nowyBufor = czesci.pop();
    return { linie: czesci.filter(l => l.trim().length > 0), bufor: nowyBufor };
  }

  // Składa komunikaty {"cmd":"layer","idx":N,"data":{...}} (odebrane w
  // dowolnej kolejności) w pełny config {v, active, ly[]}. Rzuca błąd,
  // jeśli brakuje którejś warstwy w sekwencji albo nic nie przyszło.
  function zlozKonfigZWarstw(fragmentyWarstw, wersja, aktywna) {
    if (!fragmentyWarstw || fragmentyWarstw.length === 0) {
      throw new Error("urządzenie nie zwróciło żadnej warstwy");
    }
    const maxIdx = fragmentyWarstw.reduce((m, f) => Math.max(m, f.idx), -1);
    const ly = new Array(maxIdx + 1).fill(null);
    fragmentyWarstw.forEach(f => { ly[f.idx] = f.data; });
    if (ly.some(l => l === null)) {
      throw new Error("brakuje niektórych warstw w odpowiedzi urządzenia");
    }
    return { v: wersja || 1, active: aktywna || 0, ly };
  }

  const api = {
    ramkaPing, ramkaGetConfig, ramkaSetLayer, ramkaSave, ramkaSetActive,
    podzielNaLinie, zlozKonfigZWarstw,
  };

  if (typeof module !== "undefined" && module.exports) {
    module.exports = api; // Node (require, testy)
  } else {
    Object.assign(root, api); // przeglądarka — funkcje trafiają do zasięgu globalnego
  }
})(typeof window !== "undefined" ? window : globalThis);
