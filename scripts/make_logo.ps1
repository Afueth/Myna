<#
Regenerates the logo assets from assets/myna-logo.png:
  src/cli/logo.c    the colour logo Myna draws when it starts in a terminal
  assets/myna.ico   the Windows icon embedded in myna.exe and used by the shortcut

Run from the project root after changing the logo:
  powershell -ExecutionPolicy Bypass -File scripts/make_logo.ps1
#>
param([int]$Columns = 40)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

public static class MynaLogo
{
    // Near-white pixels connected to the image border are background. White
    // inside the bird (the eye highlight) is kept.
    const int SoftEdge = 215;
    const int FullyClear = 250;

    static byte[] ReadPixels(Bitmap bmp, out int stride)
    {
        BitmapData data = bmp.LockBits(new Rectangle(0, 0, bmp.Width, bmp.Height),
                                       ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
        stride = data.Stride;
        byte[] px = new byte[data.Stride * bmp.Height];
        Marshal.Copy(data.Scan0, px, 0, px.Length);
        bmp.UnlockBits(data);
        return px;
    }

    static void WritePixels(Bitmap bmp, byte[] px)
    {
        BitmapData data = bmp.LockBits(new Rectangle(0, 0, bmp.Width, bmp.Height),
                                       ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
        Marshal.Copy(px, 0, data.Scan0, px.Length);
        bmp.UnlockBits(data);
    }

    // Copy of `src` with its white background made transparent.
    static Bitmap RemoveBackground(Bitmap src)
    {
        int w = src.Width, h = src.Height, stride;
        Bitmap bmp = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        using (Graphics g = Graphics.FromImage(bmp))
            g.DrawImage(src, new Rectangle(0, 0, w, h));

        byte[] px = ReadPixels(bmp, out stride);
        bool[] seen = new bool[w * h];
        Queue<int> queue = new Queue<int>();

        for (int x = 0; x < w; x++) { queue.Enqueue(x); queue.Enqueue((h - 1) * w + x); }
        for (int y = 0; y < h; y++) { queue.Enqueue(y * w); queue.Enqueue(y * w + w - 1); }

        while (queue.Count > 0) {
            int p = queue.Dequeue();
            if (seen[p]) continue;
            int x = p % w, y = p / w;
            int i = y * stride + x * 4;
            int m = Math.Min(px[i], Math.Min(px[i + 1], px[i + 2]));
            if (m <= SoftEdge) continue;
            seen[p] = true;

            int alpha = m >= FullyClear ? 0 : (FullyClear - m) * 255 / (FullyClear - SoftEdge);
            px[i + 3] = (byte)Math.Min((int)px[i + 3], alpha);

            if (x > 0) queue.Enqueue(p - 1);
            if (x < w - 1) queue.Enqueue(p + 1);
            if (y > 0) queue.Enqueue(p - w);
            if (y < h - 1) queue.Enqueue(p + w);
        }

        WritePixels(bmp, px);
        return bmp;
    }

    static Rectangle OpaqueBounds(Bitmap bmp)
    {
        int stride;
        byte[] px = ReadPixels(bmp, out stride);
        int minX = bmp.Width, minY = bmp.Height, maxX = -1, maxY = -1;
        for (int y = 0; y < bmp.Height; y++) {
            for (int x = 0; x < bmp.Width; x++) {
                if (px[y * stride + x * 4 + 3] < 128) continue;
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }
        return Rectangle.FromLTRB(minX, minY, maxX + 1, maxY + 1);
    }

    static Bitmap Scale(Bitmap src, Rectangle from, int width, int height)
    {
        Bitmap dst = new Bitmap(width, height, PixelFormat.Format32bppArgb);
        using (Graphics g = Graphics.FromImage(dst)) {
            g.Clear(Color.Transparent);
            g.InterpolationMode = InterpolationMode.HighQualityBicubic;
            g.PixelOffsetMode = PixelOffsetMode.HighQuality;
            using (ImageAttributes attrs = new ImageAttributes()) {
                attrs.SetWrapMode(WrapMode.TileFlipXY); // no fringe at the edges
                g.DrawImage(src, new Rectangle(0, 0, width, height),
                            from.X, from.Y, from.Width, from.Height, GraphicsUnit.Pixel, attrs);
            }
        }
        return dst;
    }

    // ---- terminal art: one character cell shows two pixels with half blocks ----

    const string Upper = "\\342\\226\\200"; // U+2580 upper half block
    const string Lower = "\\342\\226\\204"; // U+2584 lower half block
    const string DefaultBg = "\\033[49m";

    static string Fg(Color c) { return "\\033[38;2;" + c.R + ";" + c.G + ";" + c.B + "m"; }
    static string Bg(Color c) { return "\\033[48;2;" + c.R + ";" + c.G + ";" + c.B + "m"; }

    static string ArtLine(Bitmap art, int y)
    {
        StringBuilder sb = new StringBuilder();
        string fg = null, bg = null;

        for (int x = 0; x < art.Width; x++) {
            Color top = art.GetPixel(x, y), bottom = art.GetPixel(x, y + 1);
            bool topClear = top.A < 128, bottomClear = bottom.A < 128;
            string wantFg = fg, wantBg = DefaultBg, glyph = " ";

            if (!topClear && !bottomClear) { wantFg = Fg(top); wantBg = Bg(bottom); glyph = Upper; }
            else if (!topClear) { wantFg = Fg(top); glyph = Upper; }
            else if (!bottomClear) { wantFg = Fg(bottom); glyph = Lower; }

            if (wantBg != bg) { sb.Append(wantBg); bg = wantBg; }
            if (wantFg != fg) { sb.Append(wantFg); fg = wantFg; }
            sb.Append(glyph);
        }
        sb.Append("\\033[0m");
        return sb.ToString();
    }

    public static void WriteTerminalArt(Bitmap src, int columns, string path)
    {
        using (Bitmap clear = RemoveBackground(src)) {
            Rectangle crop = OpaqueBounds(clear);
            int pixelRows = (int)Math.Round(columns * (double)crop.Height / crop.Width / 2.0) * 2;
            using (Bitmap art = Scale(clear, crop, columns, pixelRows)) {
                StringBuilder sb = new StringBuilder();
                sb.Append("/* Generated by scripts/make_logo.ps1 from assets/myna-logo.png. Do not edit. */\n\n");
                sb.Append("#include \"logo.h\"\n\n");
                sb.Append("const char *const MYNA_LOGO[] = {\n");
                for (int y = 0; y < pixelRows; y += 2)
                    sb.Append("    \"").Append(ArtLine(art, y)).Append("\",\n");
                sb.Append("};\n\n");
                sb.Append("const int MYNA_LOGO_LINES = ").Append(pixelRows / 2).Append(";\n");
                sb.Append("const int MYNA_LOGO_WIDTH = ").Append(columns).Append(";\n");
                File.WriteAllText(path, sb.ToString(), new UTF8Encoding(false));
            }
        }
    }

    // ---- Windows icon: a square, transparent image at several sizes ----

    public static void WriteIcon(Bitmap src, string path)
    {
        int[] sizes = { 256, 64, 48, 32, 16 };
        List<byte[]> images = new List<byte[]>();

        using (Bitmap clear = RemoveBackground(src)) {
            Rectangle c = OpaqueBounds(clear);
            int side = Math.Max(c.Width, c.Height) * 17 / 16; // small margin
            using (Bitmap square = new Bitmap(side, side, PixelFormat.Format32bppArgb)) {
                using (Graphics g = Graphics.FromImage(square)) {
                    g.Clear(Color.Transparent);
                    g.DrawImage(clear, new Rectangle((side - c.Width) / 2, (side - c.Height) / 2, c.Width, c.Height),
                                c, GraphicsUnit.Pixel);
                }
                foreach (int s in sizes) {
                    using (Bitmap b = Scale(square, new Rectangle(0, 0, side, side), s, s))
                    using (MemoryStream ms = new MemoryStream()) {
                        b.Save(ms, ImageFormat.Png);
                        images.Add(ms.ToArray());
                    }
                }
            }
        }

        using (FileStream fs = File.Create(path))
        using (BinaryWriter w = new BinaryWriter(fs)) {
            w.Write((short)0);            // reserved
            w.Write((short)1);            // type: icon
            w.Write((short)sizes.Length);
            int offset = 6 + 16 * sizes.Length;
            for (int i = 0; i < sizes.Length; i++) {
                byte dim = (byte)(sizes[i] >= 256 ? 0 : sizes[i]);
                w.Write(dim); w.Write(dim);
                w.Write((byte)0); w.Write((byte)0);
                w.Write((short)1);        // colour planes
                w.Write((short)32);       // bits per pixel
                w.Write(images[i].Length);
                w.Write(offset);
                offset += images[i].Length;
            }
            foreach (byte[] img in images)
                w.Write(img);
        }
    }
}
'@

$src = [System.Drawing.Bitmap]::FromFile((Join-Path $root 'assets\myna-logo.png'))
try {
    [MynaLogo]::WriteTerminalArt($src, $Columns, (Join-Path $root 'src\cli\logo.c'))
    [MynaLogo]::WriteIcon($src, (Join-Path $root 'assets\myna.ico'))
} finally {
    $src.Dispose()
}
Write-Output 'Wrote src/cli/logo.c and assets/myna.ico'
