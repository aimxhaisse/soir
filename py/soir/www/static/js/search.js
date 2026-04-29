(function () {
  'use strict';

  const SEARCH_INDEX_URL = '/search-index.json';
  const DEBOUNCE_MS = 150;
  const MAX_RESULTS = 10;

  let index = [];
  let indexLoaded = false;
  let debounceTimer = null;

  const input = document.getElementById('search-input');
  const resultsContainer = document.getElementById('search-results');

  if (!input || !resultsContainer) return;

  function loadIndex() {
    if (indexLoaded) return Promise.resolve();
    return fetch(SEARCH_INDEX_URL)
      .then(r => r.json())
      .then(data => {
        index = data;
        indexLoaded = true;
      })
      .catch(() => {
        index = [];
        indexLoaded = true;
      });
  }

  function score(query, item) {
    const q = query.toLowerCase();
    const name = item.name.toLowerCase();
    const module = (item.module || '').toLowerCase();

    if (name === q) return 1000;
    if (name.startsWith(q)) return 500;
    if (name.includes(q)) return 300;

    // Fuzzy: each char of query must appear in order
    let qi = 0;
    for (let i = 0; i < name.length && qi < q.length; i++) {
      if (name[i] === q[qi]) qi++;
    }
    if (qi === q.length) return 200;

    if (module.includes(q)) return 100;

    return 0;
  }

  function search(query) {
    const q = query.trim();
    if (!q) return [];

    const scored = [];
    for (const item of index) {
      const s = score(q, item);
      if (s > 0) {
        scored.push({ item, score: s });
      }
    }

    scored.sort((a, b) => b.score - a.score);
    return scored.slice(0, MAX_RESULTS).map(x => x.item);
  }

  function renderResults(results) {
    if (results.length === 0) {
      resultsContainer.innerHTML = '<div class="search-no-results">No results</div>';
      resultsContainer.classList.add('visible');
      return;
    }

    const html = results.map((item, i) => {
      const typeLabel = item.type === 'method' ? 'method' : item.type;
      const moduleLabel = item.type === 'module' ? '' : `<span class="search-result-module">${escapeHtml(item.module)}</span>`;
      return `<a href="${escapeHtml(item.url)}" class="search-result${i === 0 ? ' active' : ''}" data-index="${i}" role="option">
        <span class="search-result-name">${escapeHtml(item.name)}</span>
        <span class="search-result-meta">
          <span class="search-result-type">${escapeHtml(typeLabel)}</span>
          ${moduleLabel}
        </span>
      </a>`;
    }).join('');

    resultsContainer.innerHTML = html;
    resultsContainer.classList.add('visible');
  }

  function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
  }

  function clearResults() {
    resultsContainer.innerHTML = '';
    resultsContainer.classList.remove('visible');
  }

  function getActiveResult() {
    return resultsContainer.querySelector('.search-result.active');
  }

  function setActiveResult(el) {
    const current = getActiveResult();
    if (current) current.classList.remove('active');
    if (el) el.classList.add('active');
  }

  function navigateResults(direction) {
    const items = resultsContainer.querySelectorAll('.search-result');
    if (items.length === 0) return;

    const current = getActiveResult();
    let idx = current ? parseInt(current.dataset.index, 10) : -1;
    idx += direction;
    if (idx < 0) idx = items.length - 1;
    if (idx >= items.length) idx = 0;

    setActiveResult(items[idx]);
    items[idx].scrollIntoView({ block: 'nearest' });
  }

  function activateCurrentResult() {
    const current = getActiveResult();
    if (current) {
      window.location.href = current.href;
    }
  }

  input.addEventListener('input', () => {
    clearTimeout(debounceTimer);
    debounceTimer = setTimeout(() => {
      loadIndex().then(() => {
        const results = search(input.value);
        renderResults(results);
      });
    }, DEBOUNCE_MS);
  });

  input.addEventListener('keydown', (e) => {
    if (!resultsContainer.classList.contains('visible')) return;

    switch (e.key) {
      case 'ArrowDown':
        e.preventDefault();
        navigateResults(1);
        break;
      case 'ArrowUp':
        e.preventDefault();
        navigateResults(-1);
        break;
      case 'Enter':
        e.preventDefault();
        activateCurrentResult();
        break;
      case 'Escape':
        clearResults();
        input.blur();
        break;
    }
  });

  document.addEventListener('click', (e) => {
    if (!input.contains(e.target) && !resultsContainer.contains(e.target)) {
      clearResults();
    }
  });

  document.addEventListener('keydown', (e) => {
    // Cmd+K or Ctrl+K or / to focus search
    if ((e.metaKey || e.ctrlKey) && e.key === 'k') {
      e.preventDefault();
      input.focus();
    }
    if (e.key === '/' && document.activeElement !== input) {
      e.preventDefault();
      input.focus();
    }
  });

})();
