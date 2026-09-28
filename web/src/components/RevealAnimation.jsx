import { useMemo } from "react";
import { motion, useReducedMotion } from "framer-motion";

function scattered(count, xRange, yRange) {
  return Array.from({ length: count }, (_, i) => ({
    id: i,
    x: xRange[0] + Math.random() * (xRange[1] - xRange[0]),
    y: yRange[0] + Math.random() * (yRange[1] - yRange[0]),
    size: 3 + Math.random() * 5,
    duration: 1 + Math.random() * 0.9,
    delay: Math.random() * 0.25,
  }));
}

// ---------------------------------------------------------------------------
// How slowly the ASCII image drops in, in seconds. The artwork descends
// gently while a top-to-bottom wipe uncovers it line by line — like the
// terminal --speed drop, but cinematic. Tweak freely (3 = graceful,
// 5 = very slow, 8 = dramatic).
// ---------------------------------------------------------------------------
const DROP_DURATION = 5;

/**
 * The surprise: light burst + rising particles & hearts, then the
 * artwork springs in with a glow, followed by the caption and a
 * deliberately quiet Replay button.
 */
export default function RevealAnimation({
  imageSrc,
  imageReady,
  imageMissing,
  onReplay,
}) {
  const reduceMotion = useReducedMotion();
  const particles = useMemo(() => scattered(22, [-160, 160], [40, 200]), []);
  const hearts = useMemo(() => scattered(8, [-190, 190], [60, 220]), []);
  const anim = (fast) => (reduceMotion ? { duration: 0.01 } : fast);

  return (
    <div className="relative flex w-full flex-col items-center px-4">
      {/* Light burst */}
      <motion.div
        aria-hidden="true"
        initial={{ opacity: 0, scale: 0.4 }}
        animate={{ opacity: [0, 0.9, 0], scale: 1.7 }}
        transition={{ duration: 1.1, ease: "easeOut" }}
        className="pointer-events-none absolute top-1/3 h-72 w-72 rounded-full"
        style={{
          background:
            "radial-gradient(circle, rgba(244,63,94,0.35) 0%, rgba(244,63,94,0.08) 45%, transparent 70%)",
          filter: "blur(10px)",
        }}
      />

      {/* Rising light particles */}
      {particles.map((p) => (
        <motion.span
          key={`p-${p.id}`}
          aria-hidden="true"
          initial={{ opacity: 0, x: 0, y: 0, scale: 0.6 }}
          animate={{ opacity: [0, 0.9, 0], x: p.x, y: -p.y, scale: 1 }}
          transition={{ duration: p.duration, delay: p.delay, ease: "easeOut" }}
          className="pointer-events-none absolute top-1/2 rounded-full bg-rose-200"
          style={{ width: p.size, height: p.size, filter: "blur(1px)" }}
        />
      ))}

      {/* A few floating hearts, kept subtle */}
      {hearts.map((h) => (
        <motion.span
          key={`h-${h.id}`}
          aria-hidden="true"
          initial={{ opacity: 0, x: 0, y: 0, scale: 0.5 }}
          animate={{ opacity: [0, 0.55, 0], x: h.x, y: -h.y, scale: 1 }}
          transition={{ duration: h.duration + 0.4, delay: h.delay, ease: "easeOut" }}
          className="pointer-events-none absolute top-1/2 text-rose-400"
          style={{ fontSize: h.size + 8 }}
        >
          ♥
        </motion.span>
      ))}

      {/* Artwork: a slow, gentle drop. The frame descends while a
          top-to-bottom wipe uncovers the ASCII image line by line. */}
      {imageReady && !imageMissing && (
        <motion.figure
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          transition={reduceMotion ? { duration: 0.01 } : { duration: 0.5 }}
          className="relative z-10 flex flex-col items-center"
        >
          <motion.div
            initial={{ y: -70, opacity: 0 }}
            animate={{ y: 0, opacity: 1 }}
            transition={
              reduceMotion
                ? { duration: 0.01 }
                : { delay: 0.5, duration: 2.8, ease: [0.22, 1, 0.36, 1] }
            }
          >
            <motion.div
              initial={{ clipPath: "inset(0 0 100% 0)" }}
              animate={{ clipPath: "inset(0 0 0% 0)" }}
              transition={
                reduceMotion
                  ? { duration: 0.01 }
                  : {
                      delay: 0.5,
                      duration: DROP_DURATION,
                      ease: [0.4, 0, 0.2, 1],
                    }
              }
              className="overflow-hidden rounded-2xl ring-1 ring-rose-400/40"
              style={{
                boxShadow:
                  "0 0 24px rgba(244,63,94,0.35), 0 0 90px rgba(244,63,94,0.18), 0 25px 60px rgba(0,0,0,0.6)",
              }}
            >
              <img
                src={imageSrc}
                alt="A special surprise"
                className="h-auto max-h-[62vh] w-auto max-w-[88vw] object-contain"
                draggable={false}
              />
            </motion.div>
          </motion.div>
          <motion.figcaption
            initial={{ opacity: 0, y: 10 }}
            animate={{ opacity: 1, y: 0 }}
            transition={anim({
              delay: reduceMotion ? 0 : 0.5 + DROP_DURATION * 0.75,
              duration: 0.7,
            })}
            className="mt-6 text-center text-sm font-light tracking-wide text-zinc-300"
          >
            Made with a little code and a lot of love. ❤️
          </motion.figcaption>
          <motion.button
            type="button"
            onClick={onReplay}
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            transition={anim({
              delay: reduceMotion ? 0 : 0.5 + DROP_DURATION + 0.4,
              duration: 0.7,
            })}
            className="mt-4 rounded-full border border-white/10 px-4 py-1.5 text-xs font-light tracking-widest text-zinc-500 uppercase transition-colors outline-none hover:border-rose-400/40 hover:text-rose-200 focus-visible:ring-2 focus-visible:ring-rose-400/60"
          >
            Replay
          </motion.button>
        </motion.figure>
      )}

      {/* Graceful fallback when the owner hasn't added the image yet */}
      {imageMissing && (
        <motion.p
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          transition={anim({ delay: 0.6, duration: 0.6 })}
          className="relative z-10 max-w-xs text-center text-sm font-light text-zinc-400"
        >
          The surprise isn&apos;t framed yet — place your image at{" "}
          <code className="text-rose-200">public/image.jpg</code> and
          reload. ❤️
        </motion.p>
      )}
    </div>
  );
}
