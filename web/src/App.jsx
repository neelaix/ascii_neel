import { useCallback, useEffect, useState } from "react";
import { AnimatePresence, motion, useReducedMotion } from "framer-motion";
import ParticleBackground from "./components/ParticleBackground.jsx";
import MagicInput from "./components/MagicInput.jsx";
import RevealAnimation from "./components/RevealAnimation.jsx";

// ---------------------------------------------------------------------------
// Single configuration point for the revealed artwork.
// Place your image at:  web/public/image.jpg  (any browser-supported
// format or dimensions — JPG, PNG, or your CrushASCII render; just update
// the file name below if you rename it).
// It is requested from the network ONLY after the magic phrase is typed.
// ---------------------------------------------------------------------------
const REVEAL_IMAGE = `${import.meta.env.BASE_URL}image.jpg`;

export default function App() {
  const [phase, setPhase] = useState("landing"); // landing | transition | revealed
  const [imageReady, setImageReady] = useState(false);
  const [imageMissing, setImageMissing] = useState(false);
  const reduceMotion = useReducedMotion();

  const handleMatch = useCallback(() => {
    setPhase((prev) => (prev === "landing" ? "transition" : prev));
  }, []);

  // Preload the artwork only after the trigger fires.
  useEffect(() => {
    if (phase !== "transition") return;
    let cancelled = false;
    const img = new Image();
    img.onload = () => {
      if (cancelled) return;
      setImageReady(true);
      setPhase("revealed");
    };
    img.onerror = () => {
      if (cancelled) return;
      setImageMissing(true);
      setPhase("revealed");
    };
    img.src = REVEAL_IMAGE;
    return () => {
      cancelled = true;
    };
  }, [phase]);

  const handleReplay = useCallback(() => {
    setImageReady(false);
    setImageMissing(false);
    setPhase("landing");
  }, []);

  const fade = reduceMotion
    ? { duration: 0.01 }
    : { duration: 0.6, ease: "easeInOut" };

  return (
    <div className="relative flex min-h-full flex-col overflow-hidden bg-[#050507]">
      {/* Cinematic ambient background */}
      <ParticleBackground />
      <div
        aria-hidden="true"
        className="animate-glow-drift pointer-events-none absolute -top-32 left-1/2 h-96 w-[42rem] max-w-none -translate-x-1/2 rounded-full bg-rose-600/10 blur-3xl"
      />
      <div
        aria-hidden="true"
        className="pointer-events-none absolute inset-0"
        style={{
          background:
            "radial-gradient(ellipse 80% 60% at 50% 110%, rgba(244,63,94,0.07), transparent 70%)",
        }}
      />

      <main className="relative z-10 flex flex-1 items-center justify-center px-4 py-16">
        <AnimatePresence mode="wait">
          {phase === "landing" ? (
            <motion.section
              key="landing"
              exit={{ opacity: 0, y: -18, filter: "blur(8px)" }}
              transition={fade}
              aria-live="polite"
              className="flex w-full flex-col items-center text-center"
            >
              <motion.p
                initial={{ opacity: 0, y: 14 }}
                animate={{ opacity: 1, y: 0 }}
                transition={reduceMotion ? { duration: 0.01 } : { duration: 0.8 }}
                className="mb-3 text-xs font-light tracking-[0.35em] text-rose-300/80 uppercase"
              >
                For you
              </motion.p>
              <motion.h1
                initial={{ opacity: 0, y: 16 }}
                animate={{ opacity: 1, y: 0 }}
                transition={reduceMotion ? { duration: 0.01 } : { duration: 0.9, delay: 0.1 }}
                className="font-display max-w-2xl text-4xl leading-tight font-medium text-zinc-50 sm:text-5xl md:text-6xl"
                style={{ textWrap: "balance" }}
              >
                Something special is waiting for you…
              </motion.h1>
              <motion.div
                initial={{ opacity: 0, y: 16 }}
                animate={{ opacity: 1, y: 0 }}
                transition={reduceMotion ? { duration: 0.01 } : { duration: 0.9, delay: 0.25 }}
                className="mt-10 flex w-full justify-center"
              >
                <MagicInput onMatch={handleMatch} />
              </motion.div>
            </motion.section>
          ) : (
            <motion.div
              key="reveal"
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              transition={fade}
              className="flex w-full justify-center"
            >
              <RevealAnimation
                imageSrc={REVEAL_IMAGE}
                imageReady={imageReady}
                imageMissing={imageMissing}
                onReplay={handleReplay}
              />
            </motion.div>
          )}
        </AnimatePresence>
      </main>
    </div>
  );
}
