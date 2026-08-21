"""Wycina rysunki SVG z instrukcji HTML do osobnych plikow i tworzy wersje
zrodlowa do konwersji na Markdown. Uruchom z katalogu glownego projektu."""
import re, pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / 'print/instrukcja-montazu.html'
NAMES = ['00-okladka', '01-schemat-polaczen', '02-rozmieszczenie-w-obudowie',
         '03-lancuch-masy', '04-wiazka-serwisowa']

html = SRC.read_text(encoding='utf-8')
blocks = re.findall(r'<svg\b.*?</svg>', html, flags=re.S)
out = ROOT / 'hardware/diagrams'
out.mkdir(parents=True, exist_ok=True)
new = html
for i, b in enumerate(blocks):
    name = NAMES[i] if i < len(NAMES) else f'{i:02d}-rysunek'
    svg = b if 'xmlns=' in b.split('>')[0] else b.replace('<svg', '<svg xmlns="http://www.w3.org/2000/svg"', 1)
    (out / f'{name}.svg').write_text(svg, encoding='utf-8')
    new = new.replace(b, f'<p><img src="../hardware/diagrams/{name}.svg" alt="{name}"></p>')
(ROOT / 'tools/_md_src.html').write_text(new, encoding='utf-8')
print(f'zapisano {len(blocks)} rysunkow do {out}')
