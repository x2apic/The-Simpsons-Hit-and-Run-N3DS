#!/usr/bin/env python3
"""Build the New 3DS SD card folder from the PC release of the game.

    python tools/3ds/setup_sd.py "<PC game folder>"

Writes sdcard/3ds/simpsons/ (data copied as is, movies converted for the
hardware H.264 decoder) and refreshes romfs/ for the build. Incremental.
Needs ffmpeg with libx264: `pip install imageio-ffmpeg`, PATH, or --ffmpeg.
"""

import argparse
import concurrent.futures
import fractions
import os
import pathlib
import re
import shutil
import struct
import subprocess
import sys
import tempfile

REPO = pathlib.Path(__file__).resolve().parents[2]

COPY_DIRS = ("art", "scripts", "sound")
ROMFS_DIR = pathlib.PurePosixPath("art/frontend/scrooby")
SKIP_NAMES = {"temp.err"}

MOVIE_MAGIC = b"SRR3DSMV"
MOVIE_VERSION = 1
MOVIE_HEADER = struct.Struct("<8sIHHIIIIHHIIII")
MOVIE_WIDTH, MOVIE_HEIGHT = 320, 240
AUDIO_RATE = 32000
ENCODE_STAMP = "x264 baseline L3.0 crf22 refs1 bf0 / pcm16 32k v1"


def log(msg):
    print(msg, flush=True)


def find_ffmpeg(explicit):
    if explicit:
        return explicit
    try:
        import imageio_ffmpeg
        return imageio_ffmpeg.get_ffmpeg_exe()
    except ImportError:
        pass
    found = shutil.which("ffmpeg")
    if found:
        return found
    sys.exit("ffmpeg not found: run `pip install imageio-ffmpeg`, put ffmpeg on PATH, or pass --ffmpeg.")


def probe(ffmpeg, path):
    out = subprocess.run([ffmpeg, "-hide_banner", "-i", str(path)],
                         capture_output=True, text=True, errors="replace").stderr
    video = next((l for l in out.splitlines() if "Video:" in l), None)
    if video is None:
        raise RuntimeError(f"{path.name}: no video stream")
    m = re.search(r"([\d.]+) fps", video) or re.search(r"([\d.]+) tbr", video)
    fps = float(m.group(1)) if m else 30.0
    for num, den in ((30000, 1001), (24000, 1001), (60000, 1001)):
        if abs(fps - num / den) < 0.01:
            break
    else:
        f = fractions.Fraction(fps).limit_denominator(1000)
        num, den = f.numerator, f.denominator
    return (num, den), ("Audio:" in out)


def split_annex_b(data):
    starts = []
    i = data.find(b"\0\0\1")
    while i >= 0:
        starts.append(i + 3)
        i = data.find(b"\0\0\1", i + 3)
    for n, s in enumerate(starts):
        end = starts[n + 1] - 3 if n + 1 < len(starts) else len(data)
        payload = data[s:end].rstrip(b"\0")
        if payload:
            yield b"\0\0\1" + payload


def convert_movie(ffmpeg, src, dst):
    (fps_num, fps_den), has_audio = probe(ffmpeg, src)
    with tempfile.TemporaryDirectory(prefix="srr2-movie-") as tmp:
        h264 = pathlib.Path(tmp) / "video.h264"
        pcm = pathlib.Path(tmp) / "audio.pcm"
        subprocess.run([
            ffmpeg, "-hide_banner", "-loglevel", "error", "-y", "-i", str(src), "-an",
            "-vf", f"fps={fps_num}/{fps_den},scale={MOVIE_WIDTH}:{MOVIE_HEIGHT}:flags=lanczos",
            "-c:v", "libx264", "-profile:v", "baseline", "-level:v", "3.0",
            "-pix_fmt", "yuv420p", "-preset", "slow", "-crf", "22",
            "-maxrate", "1200k", "-bufsize", "2400k",
            "-g", "60", "-bf", "0", "-refs", "1", "-x264-params", "slices=1",
            "-f", "h264", str(h264),
        ], check=True)
        audio = b""
        if has_audio:
            subprocess.run([
                ffmpeg, "-hide_banner", "-loglevel", "fatal", "-y", "-i", str(src), "-vn",
                "-map", "0:a:0", "-ac", "2", "-ar", str(AUDIO_RATE),
                "-c:a", "pcm_s16le", "-f", "s16le", str(pcm),
            ], check=True)
            audio = pcm.read_bytes()

        nals = list(split_annex_b(h264.read_bytes()))
        frames = sum(1 for n in nals if (n[3] & 0x1F) in (1, 5))
        if frames == 0:
            raise RuntimeError(f"{src.name}: the encoder produced no pictures")

        video = bytearray()
        for nal in nals:
            video += struct.pack("<I", len(nal)) + nal
        video += struct.pack("<I", 0)

        audio_offset = MOVIE_HEADER.size
        video_offset = audio_offset + len(audio)
        header = MOVIE_HEADER.pack(
            MOVIE_MAGIC, MOVIE_VERSION, MOVIE_WIDTH, MOVIE_HEIGHT, fps_num, fps_den, frames,
            AUDIO_RATE if audio else 0, 2 if audio else 0, 0,
            audio_offset, len(audio), video_offset, len(video))
        part = dst.with_suffix(".part")
        with part.open("wb") as f:
            f.write(header)
            f.write(audio)
            f.write(video)
        os.replace(part, dst)
        stamp(dst).write_text(ENCODE_STAMP)
    return frames, fps_num / fps_den, dst.stat().st_size


def stamp(movie):
    return movie.with_suffix(".3dm.txt")


def movie_up_to_date(src, dst):
    s = stamp(dst)
    return (dst.exists() and s.exists() and s.read_text() == ENCODE_STAMP
            and dst.stat().st_mtime >= src.stat().st_mtime)


def same_file(src, dst):
    if not dst.exists():
        return False
    a, b = src.stat(), dst.stat()
    return a.st_size == b.st_size and int(a.st_mtime) == int(b.st_mtime)


def mirror(src_root, dst_root, rel_dirs, label):
    copied = removed = 0
    for rel in rel_dirs:
        src_dir, dst_dir = src_root / rel, dst_root / rel
        wanted = set()
        for src in src_dir.rglob("*"):
            if src.is_dir() or src.name in SKIP_NAMES:
                continue
            r = src.relative_to(src_dir)
            wanted.add(r)
            dst = dst_dir / r
            if not same_file(src, dst):
                dst.parent.mkdir(parents=True, exist_ok=True)
                if dst.exists():
                    dst.chmod(0o666)
                shutil.copy2(src, dst)
                dst.chmod(0o666)
                copied += 1
        if dst_dir.exists():
            for dst in sorted(dst_dir.rglob("*"), reverse=True):
                if dst.is_file() and dst.relative_to(dst_dir) not in wanted:
                    log(f"  removing {label}/{rel}/{dst.relative_to(dst_dir)} (not in the PC release)")
                    dst.chmod(0o666)
                    dst.unlink()
                    removed += 1
                elif dst.is_dir() and not any(dst.iterdir()):
                    dst.rmdir()
    return copied, removed


def copy_files(src_root, dst_root, names):
    copied = 0
    for name in names:
        src, dst = src_root / name, dst_root / name
        if not same_file(src, dst):
            if dst.exists():
                dst.chmod(0o666)
            shutil.copy2(src, dst)
            dst.chmod(0o666)
            copied += 1
    return copied


README = """The Simpsons Hit & Run - New Nintendo 3DS

Copy the "3ds" folder to the root of the SD card. The game must end up at
sd:/3ds/simpsons/. Launch it from the Homebrew Launcher (New 3DS only).
Sound needs sd:/3ds/dspfirm.cdc (dump it with the DSP1 homebrew).
"""


def check_game(game, allow_leak):
    missing = [n for n in ("art", "scripts", "scripts.rcf") if not (game / n).exists()]
    if missing:
        sys.exit(f"{game} does not look like the PC game folder (no {', '.join(missing)}).")
    leak = [n for n in ("Simpsons-Debug.exe", "SRR2wd.exe", "music.rcf") if (game / n).exists()]
    if leak and not allow_leak:
        sys.exit(f"{game} looks like the leaked development data ({', '.join(leak)}), which is an "
                 f"earlier version of the game. Point this at the official PC release.")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("game", type=pathlib.Path, help="the installed PC game folder")
    ap.add_argument("--out", type=pathlib.Path, default=REPO / "sdcard",
                    help="SD card root to write 3ds/simpsons/ under (default: sdcard/ in the repo)")
    ap.add_argument("--romfs", type=pathlib.Path, default=REPO / "romfs",
                    help="romfs folder the build packs into the .3dsx (default: romfs/ in the repo)")
    ap.add_argument("--no-romfs", action="store_true", help="leave romfs/ alone")
    ap.add_argument("--3dsx", dest="dsx", type=pathlib.Path, help="a built SimpsonsNTSC.3dsx to copy in")
    ap.add_argument("--no-movies", action="store_true", help="skip the movies (for a build without them)")
    ap.add_argument("--force-movies", action="store_true", help="convert every movie again")
    ap.add_argument("--jobs", type=int, default=min(4, os.cpu_count() or 1),
                    help="movies converted at once (default: up to 4)")
    ap.add_argument("--ffmpeg", help="path to ffmpeg (default: imageio-ffmpeg, then PATH)")
    ap.add_argument("--allow-leak", action="store_true", help=argparse.SUPPRESS)
    args = ap.parse_args()

    game = args.game.resolve()
    check_game(game, args.allow_leak)
    sd = args.out.resolve() / "3ds" / "simpsons"
    sd.mkdir(parents=True, exist_ok=True)

    log(f"game data: {game}")
    log(f"SD folder: {sd}")

    copied, removed = mirror(game, sd, COPY_DIRS, "sd")
    rcfs = sorted(p.name for p in game.glob("*.rcf"))
    copied += copy_files(game, sd, rcfs)
    log(f"data: {copied} files copied, {removed} removed ({len(rcfs)} archives)")

    if not args.no_romfs:
        c, r = mirror(game, args.romfs.resolve(), [str(ROMFS_DIR)], "romfs")
        log(f"romfs: {c} files copied, {r} removed" + (" - rebuild the .3dsx" if c or r else ""))

    if not args.no_movies:
        ffmpeg = find_ffmpeg(args.ffmpeg)
        movies_in = sorted((game / "movies").glob("*.rmv"))
        movies_out = sd / "movies"
        movies_out.mkdir(exist_ok=True)
        todo = [(m, movies_out / (m.stem + ".3dm")) for m in movies_in]
        todo = [(s, d) for s, d in todo if args.force_movies or not movie_up_to_date(s, d)]
        log(f"movies: {len(movies_in)} found, {len(todo)} to convert")
        failed = 0
        with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
            jobs = {pool.submit(convert_movie, ffmpeg, s, d): s for s, d in todo}
            for job in concurrent.futures.as_completed(jobs):
                src = jobs[job]
                try:
                    frames, fps, size = job.result()
                    log(f"  {src.name}: {frames} frames at {fps:.2f} fps, {size / 1e6:.1f} MB")
                except Exception as e:
                    failed += 1
                    log(f"  {src.name}: FAILED - {e}")
        if failed:
            log(f"movies: {failed} failed; those will be skipped in the game")

    if args.dsx:
        shutil.copy2(args.dsx, sd / "SimpsonsNTSC.3dsx")
        log(f"copied {args.dsx}")
    elif not (sd / "SimpsonsNTSC.3dsx").exists():
        log("no SimpsonsNTSC.3dsx yet: build it and copy it in, or pass --3dsx")

    (args.out.resolve() / "README.txt").write_text(README)
    log("done")


if __name__ == "__main__":
    main()
