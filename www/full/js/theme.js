(function () {
  const cookieName = 'blog_theme';
  const themes = new Set(['light', 'dark']);

  function readCookie(name) {
    const prefix = `${name}=`;
    const parts = document.cookie ? document.cookie.split('; ') : [];
    for (const part of parts) {
      if (part.startsWith(prefix)) {
        return decodeURIComponent(part.slice(prefix.length));
      }
    }
    return '';
  }

  function writeCookie(name, value) {
    document.cookie = `${name}=${encodeURIComponent(value)}; Path=/; Max-Age=31536000; SameSite=Lax`;
  }

  function normalizeTheme(value) {
    return themes.has(value) ? value : 'light';
  }

  function applyTheme(value) {
    const theme = normalizeTheme(value);
    const root = document.documentElement;
    root.classList.remove('theme-light', 'theme-dark');
    root.classList.add(`theme-${theme}`);
    const picker = document.getElementById('theme-picker');
    if (picker) {
      picker.value = theme;
    }
    return theme;
  }

  function currentTheme() {
    const saved = readCookie(cookieName);
    if (themes.has(saved)) {
      return saved;
    }
    if (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) {
      return 'dark';
    }
    return 'light';
  }

  const initialTheme = applyTheme(currentTheme());
  writeCookie(cookieName, initialTheme);

  window.addEventListener('DOMContentLoaded', () => {
    const picker = document.getElementById('theme-picker');
    if (picker) {
      picker.value = initialTheme;
      picker.addEventListener('change', (event) => {
        const nextTheme = applyTheme(event.target.value);
        writeCookie(cookieName, nextTheme);
      });
    }
  });
})();
