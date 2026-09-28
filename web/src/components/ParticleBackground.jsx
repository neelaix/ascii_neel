import { useEffect, useRef } from "react";

/**
 * Ambient cinematic background: twinkling stars, slow dust motes and a
 * handful of faint drifting hearts, all painted on one canvas with a
 * capped pixel ratio and a single rAF loop. No external assets.
 */
const STAR_COUNT = 90;
const DUST_COUNT = 28;
const HEART_COUNT = 6;

function rand(min, max) {
  return min + Math.random() * (max - min);
}

export default function ParticleBackground() {
  const canvasRef = useRef(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    const reduceMotion = window.matchMedia(
      "(prefers-reduced-motion: reduce)",
    ).matches;

    let width = 0;
    let height = 0;
    let raf = 0;
    let stars = [];
    let dust = [];
    let hearts = [];

    const seed = () => {
      const area = (width * height) / (1280 * 800);
      const density = Math.min(1.4, Math.max(0.5, area));
      stars = Array.from({ length: Math.round(STAR_COUNT * density) }, () => ({
        x: Math.random() * width,
        y: Math.random() * height,
        r: rand(0.4, 1.4),
        phase: rand(0, Math.PI * 2),
        speed: rand(0.4, 1.2),
      }));
      dust = Array.from({ length: Math.round(DUST_COUNT * density) }, () => ({
        x: Math.random() * width,
        y: Math.random() * height,
        r: rand(1, 2.6),
        vx: rand(-0.08, 0.08),
        vy: rand(-0.12, -0.03),
        alpha: rand(0.04, 0.12),
      }));
      hearts = Array.from({ length: HEART_COUNT }, () => ({
        x: Math.random() * width,
        y: Math.random() * height,
        size: rand(9, 17),
        vy: rand(-0.16, -0.07),
        sway: rand(0.2, 0.7),
        phase: rand(0, Math.PI * 2),
        alpha: rand(0.05, 0.11),
      }));
    };

    const resize = () => {
      const dpr = Math.min(window.devicePixelRatio || 1, 2);
      width = window.innerWidth;
      height = window.innerHeight;
      canvas.width = Math.floor(width * dpr);
      canvas.height = Math.floor(height * dpr);
      canvas.style.width = `${width}px`;
      canvas.style.height = `${height}px`;
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      seed();
    };

    const draw = (t) => {
      ctx.clearRect(0, 0, width, height);

      for (const s of stars) {
        const tw = 0.35 + 0.65 * Math.abs(Math.sin(t * 0.0006 * s.speed + s.phase));
        ctx.globalAlpha = 0.5 * tw;
        ctx.fillStyle = "#e8e8f0";
        ctx.beginPath();
        ctx.arc(s.x, s.y, s.r, 0, Math.PI * 2);
        ctx.fill();
      }

      for (const d of dust) {
        d.x += d.vx;
        d.y += d.vy;
        if (d.y < -8) {
          d.y = height + 8;
          d.x = Math.random() * width;
        }
        if (d.x < -8) d.x = width + 8;
        if (d.x > width + 8) d.x = -8;
        ctx.globalAlpha = d.alpha;
        ctx.fillStyle = "#fda4af";
        ctx.beginPath();
        ctx.arc(d.x, d.y, d.r, 0, Math.PI * 2);
        ctx.fill();
      }

      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      for (const h of hearts) {
        h.y += h.vy;
        const x = h.x + Math.sin(t * 0.0004 + h.phase) * 22 * h.sway;
        if (h.y < -24) {
          h.y = height + 24;
          h.x = Math.random() * width;
        }
        ctx.globalAlpha = h.alpha;
        ctx.fillStyle = "#fb7185";
        ctx.font = `${h.size}px serif`;
        ctx.fillText("♥", x, h.y);
      }

      ctx.globalAlpha = 1;
    };

    resize();
    window.addEventListener("resize", resize);

    if (reduceMotion) {
      // One calm static frame; no loop.
      draw(1200);
    } else {
      const loop = (t) => {
        if (!document.hidden) draw(t);
        raf = requestAnimationFrame(loop);
      };
      raf = requestAnimationFrame(loop);
    }

    return () => {
      cancelAnimationFrame(raf);
      window.removeEventListener("resize", resize);
    };
  }, []);

  return (
    <canvas
      ref={canvasRef}
      aria-hidden="true"
      className="pointer-events-none fixed inset-0 h-full w-full"
    />
  );
}
