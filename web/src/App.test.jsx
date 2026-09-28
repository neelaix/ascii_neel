import { describe, expect, it, vi, afterEach } from "vitest";
import { render, screen, fireEvent, waitFor, cleanup } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import App from "./App.jsx";

afterEach(() => {
  cleanup();
  vi.restoreAllMocks();
});

function getInput() {
  return screen.getByLabelText(/type the magic words/i);
}

// jsdom never loads real images, so stand in for a successful fetch.
function mockImageSuccess() {
  const RealImage = window.Image;
  window.Image = class {
    set src(_) {
      setTimeout(() => this.onload?.(), 0);
    }
  };
  return () => {
    window.Image = RealImage;
  };
}

function revealHappened() {
  return !!document.querySelector("figure img");
}

describe("magic phrase trigger", () => {
  it.each(["i love you", "I LOVE YOU", "I Love You", "  i LOVE you  "])(
    "reveals on %s",
    async (phrase) => {
      const restore = mockImageSuccess();
      try {
        render(<App />);
        await userEvent.type(getInput(), phrase);
        await waitFor(() => expect(revealHappened()).toBe(true));
        expect(
          document.querySelector("figure img").getAttribute("src"),
        ).toContain("image.jpg");
        expect(
          screen.getByText(/made with a little code and a lot of love/i),
        ).toBeTruthy();
      } finally {
        restore();
      }
    },
  );

  it.each(["i love", "i love you!", "i love you so much", "i  love", "love you"])(
    "stays calm on %s",
    async (phrase) => {
      render(<App />);
      await userEvent.type(getInput(), phrase);
      await new Promise((r) => setTimeout(r, 300));
      expect(revealHappened()).toBe(false);
      expect(getInput()).toBeTruthy();
    },
  );

  it("does not touch the image before the trigger", async () => {
    const requested = [];
    const RealImage = window.Image;
    window.Image = class extends RealImage {
      constructor() {
        super();
        requested.push("constructed");
      }
    };
    render(<App />);
    await userEvent.type(getInput(), "i love");
    expect(requested).toHaveLength(0);
    expect(document.querySelector("img")).toBeNull();
    window.Image = RealImage;
  });

  it("replay resets to the landing state", async () => {
    const restore = mockImageSuccess();
    try {
      const user = userEvent.setup();
      render(<App />);
      await user.type(getInput(), "i love you");
      await waitFor(() => expect(revealHappened()).toBe(true));
      fireEvent.click(screen.getByRole("button", { name: /replay/i }));
      await waitFor(() => expect(revealHappened()).toBe(false));
      expect(getInput()).toBeTruthy();
    } finally {
      restore();
    }
  });

  it("shows a graceful fallback when the image is missing", async () => {
    const RealImage = window.Image;
    let errorCb = null;
    window.Image = class {
      set src(_) {
        setTimeout(() => errorCb?.(), 0);
      }
      set onerror(fn) {
        errorCb = fn;
      }
    };
    render(<App />);
    fireEvent.change(getInput(), { target: { value: "i love you" } });
    await waitFor(() =>
      expect(screen.getByText(/isn't framed yet/i)).toBeTruthy(),
    );
    window.Image = RealImage;
  });
});
