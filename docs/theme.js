// Start dark, restore this site's explicit choice, and keep switching usable
// when browser privacy settings block local storage.
class PageTheme {
  constructor(root) {
    this.root = root;
    this.storageKey = root.dataset.themeKey;
    this.stylesheet = document.getElementById('dark-theme');
    this.choice = 'dark';
    this.button = null;
    try {
      const saved = localStorage.getItem(this.storageKey);
      if (saved === 'light' || saved === 'dark') this.choice = saved;
    } catch {
      // The default remains usable without browser storage.
    }
    this.apply();
    document.addEventListener('DOMContentLoaded', () => this.attach());
  }

  // Apply the theme before the body paints, then update the control if attached.
  apply() {
    const dark = this.choice === 'dark';
    this.root.dataset.theme = this.choice;
    if (this.stylesheet) this.stylesheet.media = dark ? 'all' : 'not all';
    if (!this.button) return;
    this.button.textContent = dark ? 'Light theme' : 'Dark theme';
    this.button.setAttribute('aria-label', dark ? 'Switch to light theme' : 'Switch to dark theme');
  }

  // Native buttons support keyboard activation; reveal only a working control.
  attach() {
    this.button = document.getElementById('theme-toggle');
    if (!this.button) return;
    this.button.hidden = false;
    this.button.addEventListener('click', () => this.toggle());
    this.apply();
  }

  // Remember the choice per site without requiring storage for the current page.
  toggle() {
    this.choice = this.choice === 'dark' ? 'light' : 'dark';
    try {
      localStorage.setItem(this.storageKey, this.choice);
    } catch {
      // The current page still switches when storage is blocked.
    }
    this.apply();
  }
}

new PageTheme(document.documentElement);
