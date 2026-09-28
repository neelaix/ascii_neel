import { useState } from "react";
import { motion } from "framer-motion";

export const MAGIC_PHRASE = "i love you";

function normalize(value) {
  return value.toLowerCase().trim().replace(/\s+/g, " ");
}

/** Length of the matching prefix — drives the subtle typing glow. */
function matchProgress(value) {
  const clean = value.toLowerCase().replace(/\s+/g, " ");
  let i = 0;
  while (i < clean.length && i < MAGIC_PHRASE.length && clean[i] === MAGIC_PHRASE[i]) {
    i += 1;
  }
  return i / MAGIC_PHRASE.length;
}

/**
 * The single input. No submit button: typing the exact magic phrase
 * (case-insensitive, surrounding spaces ignored) fires `onMatch` once.
 */
export default function MagicInput({ onMatch }) {
  const [value, setValue] = useState("");
  const [matched, setMatched] = useState(false);
  const progress = matchProgress(value);

  const handleChange = (event) => {
    if (matched) return;
    const next = event.target.value;
    setValue(next);
    if (normalize(next) === MAGIC_PHRASE) {
      setMatched(true);
      onMatch();
    }
  };

  return (
    <div className="w-full max-w-sm">
      <label
        htmlFor="magic-words"
        className="mb-3 block text-center text-sm font-light tracking-[0.2em] text-zinc-400 uppercase"
      >
        Type the magic words ❤️
      </label>
      <motion.div
        animate={{
          boxShadow:
            progress > 0
              ? `0 0 ${12 + progress * 28}px rgba(244, 63, 94, ${0.15 + progress * 0.35})`
              : "0 0 0px rgba(244, 63, 94, 0)",
        }}
        transition={{ duration: 0.4 }}
        className="rounded-2xl"
      >
        <input
          id="magic-words"
          type="text"
          autoComplete="off"
          autoCapitalize="off"
          spellCheck={false}
          autoFocus
          value={value}
          onChange={handleChange}
          placeholder="Type here..."
          disabled={matched}
          className="w-full rounded-2xl border border-white/10 bg-white/[0.04] px-5 py-4 text-center text-lg font-light tracking-wide text-zinc-100 placeholder:text-zinc-600 backdrop-blur-xl transition-colors duration-300 outline-none focus:border-rose-400/50 focus:bg-white/[0.06] focus-visible:ring-2 focus-visible:ring-rose-400/60 disabled:opacity-70"
        />
      </motion.div>
    </div>
  );
}
