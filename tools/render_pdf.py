#!/usr/bin/env python3
"""Renderuje print/instrukcja-montazu.html do PDF (A4, z numeracja stron).

Wymaga:  pip install playwright && playwright install chromium
Uzycie:  python3 tools/render_pdf.py
"""
import pathlib
from playwright.sync_api import sync_playwright

ROOT = pathlib.Path(__file__).resolve().parent.parent
ZRODLO = (ROOT / 'print/instrukcja-montazu.html').resolve().as_uri()
WYNIK = ROOT / 'print/instrukcja-montazu.pdf'

STOPKA = """<div style="width:100%;font-family:sans-serif;font-size:7.5pt;color:#8a8880;
padding:0 14mm;display:flex;justify-content:space-between;">
<span>Makropad BLE — instrukcja montażu</span>
<span class="pageNumber"></span></div>"""

with sync_playwright() as p:
    przegladarka = p.chromium.launch()
    strona = przegladarka.new_page()
    strona.goto(ZRODLO, wait_until='networkidle')
    strona.emulate_media(media='print')
    strona.pdf(path=str(WYNIK), format='A4', print_background=True,
               display_header_footer=True, header_template='<div></div>',
               footer_template=STOPKA,
               margin={'top': '12mm', 'bottom': '14mm', 'left': '14mm', 'right': '14mm'})
    przegladarka.close()

print(f'zapisano {WYNIK}')
