# Razer Mouse Lite website and documentation

`docs/index.html` is the standalone landing page. `docs/assets/logo.svg` is the
original vector logo for this website and its favicon. `docs/style.css` supplies
responsive layout and explicit light/dark appearance. The website uses no
external fonts, trackers or application runtime dependencies. Its only script is
the small theme control.

`USER-GUIDE.md` documents installation, profiles, settings, device support,
startup and local data. `BUILDING.md` remains the build/packaging reference.
The landing page uses the existing dark/light application screenshots under
`docs/images`; it switches between them with the website theme control.
The original mouse drawing is website artwork. The application's existing
Razer badge assets, executable icons and tray icons are unchanged.

GitHub Pages can publish the default branch's `/docs` directory. Jekyll uses
`_config.yml`, `jekyll-optional-front-matter`, `jekyll-relative-links` and the
documentation layout to turn Markdown guides into HTML. Source links outside
`docs` resolve to the repository on GitHub. Documentation stays in this project.

With Jekyll 3.10 and `kramdown-parser-gfm` installed:

```sh
jekyll build --source docs --destination build/site/razer-mouse-lite
python -m http.server 8000 --directory build/site
```

Open `http://localhost:8000/razer-mouse-lite/`. The output path includes the configured
base path so guide assets resolve correctly. These source files do not enable
GitHub Pages or publish a deployment; configure Pages separately when publishing.

## Website theme control

The homepage and documentation layout start with `data-theme="dark"`.
`theme.js` restores a saved light/dark choice before the body paints and attaches
the keyboard-accessible header button. The preference uses the site's own
`data-theme-key` in local storage; storage failures do not prevent switching.
The default is dark even when the operating system prefers light. Without
JavaScript, the pages retain their default dark appearance.
