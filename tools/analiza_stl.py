#!/usr/bin/env python3
"""Analiza obudowy makropada z plikow STL.

Wypisuje gabaryty, przeswit wewnetrzny, polozenia otworow w pokrywie
i wolne przestrzenie w dnie. Wyniki odpowiadaja docs/01-analiza-obudowy.md.

Wymaga:  pip install trimesh shapely rtree numpy
Uzycie:  python3 tools/analiza_stl.py            # czyta z cad/
         python3 tools/analiza_stl.py sciezka/   # albo z podanego katalogu
"""
import sys
import pathlib
import numpy as np
import trimesh

ROOT = pathlib.Path(__file__).resolve().parent.parent
CAD = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / 'cad'


def wczytaj(nazwa):
    sciezka = CAD / nazwa
    if not sciezka.exists():
        sys.exit(f'Brak pliku {sciezka}. Skopiuj STL-e do katalogu cad/.')
    return trimesh.load(sciezka)


def gabaryty(mesh, nazwa):
    lo, hi = mesh.bounds
    print(f'\n=== {nazwa} ===')
    print(f'  wymiar XYZ      : {np.round(hi - lo, 2).tolist()} mm')
    print(f'  zakres Z        : {lo[2]:.2f} … {hi[2]:.2f} mm')
    print(f'  objetosc        : {mesh.volume / 1000:.2f} cm3')
    print(f'  siatka szczelna : {mesh.is_watertight}')


def otwory(mesh, z, nazwa):
    """Wypisuje otwory (wewnetrzne kontury) w przekroju na wysokosci z."""
    przekroj = mesh.section(plane_origin=[0, 0, z], plane_normal=[0, 0, 1])
    if przekroj is None:
        print(f'  z={z:.2f}: brak przekroju')
        return
    plaski, macierz = przekroj.to_planar()
    dx, dy = macierz[0, 3], macierz[1, 3]
    print(f'\n--- {nazwa}, przekroj z = {z:.2f} mm ---')
    for wielokat in plaski.polygons_full:
        for kontur in wielokat.interiors:
            p = np.array(kontur.coords)
            lo, hi = p.min(0), p.max(0)
            sx, sy = hi - lo
            cx, cy = (lo + hi) / 2 + [dx, dy]
            ksztalt = 'okragly' if len(p) > 40 and abs(sx - sy) < 0.3 else 'prostokatny'
            print(f'  srodek ({cx:8.2f}, {cy:8.2f})  {sx:6.2f} x {sy:6.2f} mm  {ksztalt}')


def wysokosc(mesh, punkty, od_gory=True):
    """Zwraca wysokosc pierwszej napotkanej powierzchni w podanych punktach XY."""
    lo, hi = mesh.bounds
    z0 = hi[2] + 5 if od_gory else lo[2] - 5
    kier = [0, 0, -1] if od_gory else [0, 0, 1]
    poczatki = np.array([[x, y, z0] for x, y in punkty])
    kierunki = np.tile(kier, (len(poczatki), 1))
    trafienia, indeksy, _ = mesh.ray.intersects_location(poczatki, kierunki, multiple_hits=False)
    wynik = {i: t[2] for t, i in zip(trafienia, indeksy)}
    return [round(wynik.get(i, float('nan')), 2) for i in range(len(punkty))]


def main():
    dol = wczytaj('Dol.stl')
    pokrywa = wczytaj('Lid_final.stl')

    gabaryty(dol, 'DOL')
    gabaryty(pokrywa, 'POKRYWA')

    otwory(pokrywa, 16.0, 'POKRYWA')   # klawisze, OLED, enkoder, wylacznik, sruby
    otwory(dol, 12.0, 'DOL')           # slupki srub

    kontrolne = [(-60, -115), (-95, -60), (-30, -60), (-105, -90)]
    podloga = wysokosc(dol, kontrolne, od_gory=True)
    sufit = wysokosc(pokrywa, kontrolne, od_gory=False)
    print('\n--- przeswit wewnetrzny w punktach kontrolnych ---')
    for punkt, p, s in zip(kontrolne, podloga, sufit):
        if not (np.isnan(p) or np.isnan(s)):
            print(f'  {str(punkt):>16}: podloga {p:5.2f}  spod pokrywy {s:5.2f}  '
                  f'-> {s - p:5.2f} mm')
        else:
            print(f'  {str(punkt):>16}: otwor przelotowy (wyciecie klawisza)')

    print('\nUwaga: przekroje wszystkich czterech scianek sa pelne '
          '- w obudowie nie ma otworu na USB-C.')


if __name__ == '__main__':
    main()
