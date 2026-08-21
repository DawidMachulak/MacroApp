"""Zamienia instrukcje HTML na czysty Markdown (bez <div>-ow).
Uruchom po tools/extract_svg.py, z katalogu glownego projektu."""
import re, pathlib, subprocess

ROOT = pathlib.Path(__file__).resolve().parent.parent
html = (ROOT / 'tools/_md_src.html').read_text(encoding='utf-8')

html = re.sub(r'<div class="step-h">\s*<span class="step-n">(.*?)</span>\s*<span class="step-t">(.*?)</span>\s*</div>',
              r'<h3>\1 — \2</h3>', html, flags=re.S)
html = re.sub(r'<div class="cover-facts">.*?</div>\s*</div>', '', html, flags=re.S)
for cls in ('check', 'warn', 'danger'):
    html = re.sub(r'<div class="%s">(.*?)</div>' % cls,
                  lambda m: '<blockquote>%s</blockquote>' % m.group(1), html, flags=re.S)
html = re.sub(r'</?div[^>]*>', '', html)
html = re.sub(r'<span class="(?:eyebrow|k|v|swatch)"[^>]*>.*?</span>', '', html, flags=re.S)

(ROOT / 'tools/_clean.html').write_text(html, encoding='utf-8')
subprocess.run(['pandoc', str(ROOT / 'tools/_clean.html'), '-f', 'html', '-t', 'gfm',
                '--wrap=none', '-o', str(ROOT / 'docs/03-instrukcja-montazu.md')], check=True)
(ROOT / 'tools/_clean.html').unlink()
print('gotowe: docs/03-instrukcja-montazu.md')
