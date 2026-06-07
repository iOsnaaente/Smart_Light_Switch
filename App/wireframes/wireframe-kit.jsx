// wireframe-kit.jsx — elegant UI kit for the SmartLight screens.
// Plus Jakarta Sans · warm neutrals · amber (lamp light) + cool blue (daylight).
// Same component APIs as before, restyled. Exported to window at the bottom.

const WF = {
  paper: '#FAF8F4',
  surface: '#FFFFFF',
  ink: '#23211D',
  ink2: '#938D82',
  ink3: '#C2BCB1',
  hair: '#ECE7DD',
  hair2: '#F2EEE6',
  fill: '#F4F0E8',
  accent: '#D9942B',        // warm lamp light
  accentSoft: '#FAEDD2',
  accentSoft2: '#FCF5E8',
  accentInk: '#9A6412',
  cool: '#6E89A6',          // natural daylight
  coolSoft: '#E9EEF3',
  coolInk: '#3F5871',
  ok: '#5E9B79',
  okSoft: '#E5EFE8',
  off: '#BCB6AB',
  shadow: '0 1px 2px rgba(35,30,22,.04), 0 6px 18px rgba(35,30,22,.05)',
  shadowLg: '0 2px 6px rgba(35,30,22,.05), 0 14px 36px rgba(35,30,22,.08)',
  // legacy aliases kept so screen files keep working
  sans: "'Plus Jakarta Sans', system-ui, -apple-system, sans-serif",
};
WF.hand = WF.sans; WF.hand2 = WF.sans; WF.mono = WF.sans;
const NUM = { fontVariantNumeric: 'tabular-nums', letterSpacing: '-0.02em' };
const LBL = { textTransform: 'uppercase', letterSpacing: '0.09em', fontWeight: 600 };

// ── Phone screen shell ─────────────────────────────────────────
function Screen({ children, bg = WF.paper, pad = 0, dark = false, label }) {
  return (
    <div style={{ height: '100%', width: '100%', background: bg, display: 'flex', flexDirection: 'column',
      fontFamily: WF.sans, color: WF.ink, position: 'relative', letterSpacing: '-0.01em' }} data-screen-label={label}>
      <StatusBar dark={dark} />
      <div style={{ flex: 1, display: 'flex', flexDirection: 'column', padding: pad, minHeight: 0 }}>{children}</div>
    </div>
  );
}

function StatusBar({ dark }) {
  const c = dark ? 'rgba(255,255,255,.92)' : WF.ink;
  return (
    <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between',
      padding: '11px 20px 5px', fontSize: 12.5, fontWeight: 700, color: c, flex: '0 0 auto', ...NUM }}>
      <span>9:41</span>
      <div style={{ display: 'flex', gap: 5, alignItems: 'center' }}>
        <span style={{ display: 'inline-flex', gap: 1.5, alignItems: 'flex-end' }}>
          {[5, 7, 9, 11].map((h, i) => <span key={i} style={{ width: 2.5, height: h, borderRadius: 1, background: c, opacity: i === 3 ? 0.35 : 1 }} />)}
        </span>
        <span style={{ width: 17, height: 9, border: `1.5px solid ${c}`, borderRadius: 2.5, display: 'inline-block', position: 'relative' }}>
          <span style={{ position: 'absolute', inset: 1.5, right: 4, background: c, borderRadius: 1 }} />
        </span>
      </div>
    </div>
  );
}

// ── Top app bar ────────────────────────────────────────────────
function AppBar({ title, left, right, sub }) {
  return (
    <div style={{ display: 'flex', alignItems: 'center', gap: 10, padding: '12px 18px 10px', flex: '0 0 auto' }}>
      <div style={{ width: 30, display: 'flex', justifyContent: 'flex-start' }}>{left}</div>
      <div style={{ flex: 1, textAlign: 'center', lineHeight: 1.15 }}>
        <div style={{ fontSize: 17, fontWeight: 700 }}>{title}</div>
        {sub && <div style={{ fontSize: 11, color: WF.ink2, fontWeight: 500, marginTop: 1 }}>{sub}</div>}
      </div>
      <div style={{ width: 30, display: 'flex', justifyContent: 'flex-end' }}>{right}</div>
    </div>
  );
}

// round icon button wrapper
function IconBtn({ children, tone }) {
  return (
    <div style={{ width: 34, height: 34, borderRadius: 11, background: WF.surface, border: `1px solid ${WF.hair}`,
      display: 'flex', alignItems: 'center', justifyContent: 'center', boxShadow: '0 1px 1px rgba(35,30,22,.03)' }}>{children}</div>
  );
}

// tiny glyphs (clean unicode + drawn shapes)
function Glyph({ type, size = 18, c = WF.ink }) {
  if (type === 'menu') return (
    <span style={{ display: 'inline-flex', flexDirection: 'column', gap: 3.5 }}>
      {[14, 14, 14].map((w, i) => <span key={i} style={{ width: w, height: 1.8, borderRadius: 2, background: c }} />)}
    </span>);
  if (type === 'more') return (
    <span style={{ display: 'inline-flex', gap: 3 }}>
      {[0, 1, 2].map(i => <span key={i} style={{ width: 3.5, height: 3.5, borderRadius: '50%', background: c }} />)}
    </span>);
  const map = { back: '‹', fwd: '›', chev: '›', plus: '+', gear: '⋯', close: '✕', down: '⌄', dot: '·', check: '✓' };
  const fs = type === 'back' || type === 'fwd' || type === 'chev' ? size + 6 : size;
  return <span style={{ fontSize: fs, lineHeight: 1, color: c, fontWeight: 600, fontFamily: WF.sans }}>{map[type] || '·'}</span>;
}

// ── Generic placeholder block ──────────────────────────────────
function Box({ h = 40, w = '100%', label, dashed = false, fill = WF.fill, radius = 12, style = {} }) {
  return (
    <div style={{ width: w, height: h, background: dashed ? 'transparent' : fill,
      border: dashed ? `1.4px dashed ${WF.ink3}` : 'none',
      borderRadius: radius, display: 'flex', alignItems: 'center', justifyContent: 'center', flex: '0 0 auto',
      fontSize: 12.5, fontWeight: 500, color: WF.ink2, textAlign: 'center', ...style }}>
      {label}
    </div>
  );
}

// soft "glow" image placeholder (evokes a lamp) with a label chip
function Img({ h = 120, label = 'imagem', radius = 18, style = {} }) {
  return (
    <div style={{ width: '100%', height: h, borderRadius: radius, position: 'relative', overflow: 'hidden',
      background: `radial-gradient(120% 120% at 70% 30%, ${WF.accentSoft} 0%, ${WF.accentSoft2} 38%, ${WF.coolSoft} 100%)`,
      border: `1px solid ${WF.hair}`, display: 'flex', alignItems: 'center', justifyContent: 'center', flex: '0 0 auto', ...style }}>
      <span style={{ fontSize: 11, fontWeight: 600, color: WF.ink2, background: 'rgba(255,255,255,.78)', padding: '4px 10px',
        borderRadius: 20, backdropFilter: 'blur(2px)', ...LBL, letterSpacing: '0.06em' }}>{label}</span>
    </div>
  );
}

// placeholder copy lines
function Lines({ n = 2, w = ['100%', '70%'], gap = 7, h = 7 }) {
  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap, width: '100%' }}>
      {Array.from({ length: n }).map((_, i) => (
        <div key={i} style={{ height: h, width: Array.isArray(w) ? (w[i] ?? '85%') : w, background: WF.hair, borderRadius: 4 }} />
      ))}
    </div>
  );
}

// ── Form field ─────────────────────────────────────────────────
function Field({ label, placeholder, icon, value }) {
  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: 7, width: '100%' }}>
      {label && <span style={{ fontSize: 12, color: WF.ink2, fontWeight: 600, ...LBL, letterSpacing: '0.06em' }}>{label}</span>}
      <div style={{ display: 'flex', alignItems: 'center', gap: 9, height: 46, padding: '0 14px',
        border: `1px solid ${WF.hair}`, borderRadius: 13, background: WF.surface, boxShadow: '0 1px 1px rgba(35,30,22,.02)' }}>
        {icon && <span style={{ width: 6, height: 6, borderRadius: '50%', background: WF.ink3 }} />}
        <span style={{ fontSize: value ? 14 : 14.5, fontWeight: value ? 600 : 400,
          color: value ? WF.ink : WF.ink2, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis', ...(value ? NUM : {}) }}>{value || placeholder}</span>
      </div>
    </div>
  );
}

// ── Button ─────────────────────────────────────────────────────
function Btn({ children, primary, ghost, full, sm, style = {} }) {
  const base = { display: 'inline-flex', alignItems: 'center', justifyContent: 'center', gap: 7,
    height: sm ? 38 : 50, padding: sm ? '0 16px' : '0 22px', borderRadius: 14, cursor: 'default', whiteSpace: 'nowrap',
    fontFamily: WF.sans, fontSize: sm ? 14 : 15.5, fontWeight: 700, letterSpacing: '-0.01em',
    width: full ? '100%' : 'auto', flex: full ? '0 0 auto' : 'initial' };
  const skin = primary
    ? { background: WF.accent, color: '#fff', border: 'none', boxShadow: '0 1px 2px rgba(154,100,18,.25), 0 8px 20px rgba(217,148,43,.28)' }
    : ghost
    ? { background: 'transparent', color: WF.ink, border: `1px solid ${WF.hair}` }
    : { background: WF.surface, color: WF.ink, border: `1px solid ${WF.hair}`, boxShadow: '0 1px 1px rgba(35,30,22,.03)' };
  return <div style={{ ...base, ...skin, ...style }}>{children}</div>;
}

// ── Chips / badges / status ────────────────────────────────────
function Chip({ children, tone = 'plain' }) {
  const tones = {
    plain: { bg: WF.surface, bd: WF.hair, fg: WF.ink2 },
    on: { bg: WF.accentSoft, bd: 'transparent', fg: WF.accentInk },
    cool: { bg: WF.coolSoft, bd: 'transparent', fg: WF.coolInk },
    ok: { bg: WF.okSoft, bd: 'transparent', fg: '#3F7256' },
    off: { bg: WF.fill, bd: 'transparent', fg: WF.ink2 },
  }[tone];
  return (
    <span style={{ display: 'inline-flex', alignItems: 'center', gap: 5, fontSize: 10.5, fontWeight: 700,
      padding: '4px 10px', borderRadius: 20, background: tones.bg, border: `1px solid ${tones.bd}`, color: tones.fg,
      whiteSpace: 'nowrap', letterSpacing: '0.02em' }}>{children}</span>
  );
}

function Dot({ tone = 'ok', size = 7 }) {
  const c = { ok: WF.ok, off: WF.off, on: WF.accent, cool: WF.cool }[tone];
  return <span style={{ width: size, height: size, borderRadius: '50%', background: c, display: 'inline-block', flex: '0 0 auto' }} />;
}

// ── Segmented Manual/Auto toggle ───────────────────────────────
function Segmented({ options = ['Manual', 'Auto'], active = 0, style = {} }) {
  return (
    <div style={{ display: 'flex', padding: 4, gap: 4, background: WF.fill, borderRadius: 14, ...style }}>
      {options.map((o, i) => (
        <div key={o} style={{ flex: 1, textAlign: 'center', padding: '9px 4px', borderRadius: 10, fontSize: 14, fontWeight: i === active ? 700 : 500,
          background: i === active ? WF.surface : 'transparent',
          color: i === active ? WF.ink : WF.ink2,
          boxShadow: i === active ? '0 1px 2px rgba(35,30,22,.10)' : 'none' }}>{o}</div>
      ))}
    </div>
  );
}

// ── Horizontal slider (dimmer) ─────────────────────────────────
function Slider({ pct = 60, label, muted = false }) {
  const c = muted ? WF.ink3 : WF.accent;
  return (
    <div style={{ width: '100%' }}>
      {label && <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'baseline', gap: 8, fontSize: 10, color: WF.ink2, marginBottom: 9, ...LBL, letterSpacing: '0.05em' }}>
        <span style={{ whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>{label}</span><span style={{ color: muted ? WF.ink2 : WF.accentInk, fontWeight: 700, ...NUM }}>{pct}%</span></div>}
      <div style={{ position: 'relative', height: 10, borderRadius: 8, background: WF.fill }}>
        <div style={{ position: 'absolute', left: 0, top: 0, bottom: 0, width: pct + '%', background: muted ? WF.ink3 : `linear-gradient(90deg, ${WF.accent}, ${WF.accent})`, borderRadius: 8, opacity: muted ? 0.5 : 1 }} />
        <div style={{ position: 'absolute', left: `calc(${pct}% - 12px)`, top: -7, width: 24, height: 24, borderRadius: '50%',
          background: WF.surface, border: `2px solid ${c}`, boxShadow: '0 2px 6px rgba(35,30,22,.18)' }} />
      </div>
    </div>
  );
}

// ── Vertical fader (daylight + lamp stacked toward setpoint) ───
function Fader({ natural = 45, lamp = 30, setpoint = 75, h = 220 }) {
  return (
    <div style={{ position: 'relative', width: 78, height: h, borderRadius: 20, border: `1px solid ${WF.hair}`,
      background: WF.surface, overflow: 'hidden', flex: '0 0 auto', boxShadow: WF.shadow }}>
      <div style={{ position: 'absolute', left: 0, right: 0, bottom: 0, height: natural + '%', background: WF.coolSoft }} />
      <div style={{ position: 'absolute', left: 0, right: 0, bottom: natural + '%', height: lamp + '%',
        background: `linear-gradient(180deg, ${WF.accentSoft}, ${WF.accentSoft2})` }} />
      <div style={{ position: 'absolute', left: 0, right: 0, bottom: setpoint + '%', borderTop: `2px dashed ${WF.accentInk}` }} />
      <div style={{ position: 'absolute', left: 8, bottom: `calc(${setpoint}% + 4px)`, fontSize: 9, fontWeight: 700, color: WF.accentInk, ...LBL }}>set</div>
      {/* handle on the lamp top */}
      <div style={{ position: 'absolute', left: '50%', transform: 'translateX(-50%)', bottom: `calc(${natural + lamp}% - 4px)`, width: 30, height: 8, borderRadius: 5, background: WF.surface, border: `2px solid ${WF.accent}`, boxShadow: '0 2px 5px rgba(35,30,22,.2)' }} />
    </div>
  );
}

// ── Circular gauge (total lux toward setpoint) ─────────────────
function Dial({ size = 168, pct = 80, big = '320', unit = 'lux', cap = 'setpoint 400' }) {
  const r = size / 2 - 13, cx = size / 2, cy = size / 2, C = 2 * Math.PI * r;
  const a = 0.75;
  return (
    <div style={{ position: 'relative', width: size, height: size, flex: '0 0 auto' }}>
      <div style={{ position: 'absolute', inset: size * 0.16, borderRadius: '50%',
        background: `radial-gradient(circle, ${WF.accentSoft2} 0%, transparent 70%)`, opacity: pct > 0 ? 1 : 0 }} />
      <svg width={size} height={size}>
        <circle cx={cx} cy={cy} r={r} fill="none" stroke={WF.hair} strokeWidth="10"
          strokeDasharray={`${C * a} ${C}`} strokeLinecap="round" transform={`rotate(135 ${cx} ${cy})`} />
        {pct > 0 && <circle cx={cx} cy={cy} r={r} fill="none" stroke={WF.accent} strokeWidth="10"
          strokeDasharray={`${C * a * (pct / 100)} ${C}`} strokeLinecap="round" transform={`rotate(135 ${cx} ${cy})`} />}
      </svg>
      <div style={{ position: 'absolute', inset: 0, display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', gap: 2 }}>
        <span style={{ fontSize: size * 0.21, fontWeight: 800, color: WF.ink, lineHeight: 1, ...NUM }}>{big}</span>
        <span style={{ fontSize: 11.5, color: WF.ink2, fontWeight: 500 }}>{unit}</span>
        <span style={{ fontSize: 10, color: WF.accentInk, fontWeight: 700, marginTop: 3, ...LBL, letterSpacing: '0.04em' }}>{cap}</span>
      </div>
    </div>
  );
}

// ── Day chart: daylight (cool) vs lamp (amber) ─────────────────
function DayChart({ h = 96, caption = 'hoje' }) {
  const W = 252;
  const nat = '0,78 36,55 72,30 108,16 144,20 180,34 216,58 252,80';
  const lamp = '0,30 36,42 72,62 108,74 144,71 180,58 216,40 252,26';
  return (
    <div style={{ width: '100%' }}>
      <div style={{ position: 'relative', width: '100%', height: h, borderRadius: 12, border: `1px solid ${WF.hair}`,
        background: WF.surface, overflow: 'hidden' }}>
        <svg viewBox={`0 0 ${W} 92`} preserveAspectRatio="none" style={{ position: 'absolute', inset: 0, width: '100%', height: '100%' }}>
          <polyline points={nat} fill="none" stroke={WF.cool} strokeWidth="2.4" strokeLinecap="round" strokeLinejoin="round" />
          <polyline points={lamp} fill="none" stroke={WF.accent} strokeWidth="2.8" strokeLinecap="round" strokeLinejoin="round" />
        </svg>
      </div>
      <div style={{ display: 'flex', gap: 14, marginTop: 8, fontSize: 10, color: WF.ink2, fontWeight: 500 }}>
        <span style={{ display: 'inline-flex', alignItems: 'center', gap: 5 }}><span style={{ width: 13, height: 2.5, borderRadius: 2, background: WF.cool }} /> natural</span>
        <span style={{ display: 'inline-flex', alignItems: 'center', gap: 5 }}><span style={{ width: 13, height: 2.5, borderRadius: 2, background: WF.accent }} /> lâmpada</span>
        <span style={{ marginLeft: 'auto', ...LBL, fontSize: 9.5 }}>{caption}</span>
      </div>
    </div>
  );
}

// ── Stat tile ──────────────────────────────────────────────────
function Stat({ value, unit, label, tone }) {
  const fg = tone === 'accent' ? WF.accentInk : tone === 'cool' ? WF.coolInk : WF.ink;
  return (
    <div style={{ flex: 1, padding: '11px 8px', borderRadius: 14, border: `1px solid ${WF.hair}`, background: WF.surface, textAlign: 'center' }}>
      <div style={{ fontSize: 18, fontWeight: 800, color: fg, lineHeight: 1, ...NUM }}>
        {value}<span style={{ fontSize: 10.5, fontWeight: 600, color: WF.ink2 }}> {unit}</span></div>
      <div style={{ fontSize: 9.5, color: WF.ink2, marginTop: 5, fontWeight: 600, ...LBL, letterSpacing: '0.04em' }}>{label}</div>
    </div>
  );
}

// ── Bottom tab bar ─────────────────────────────────────────────
function TabIcon({ type, c }) {
  if (type === 'devices') return (
    <span style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 2.5 }}>
      {[0, 1, 2, 3].map(i => <span key={i} style={{ width: 5, height: 5, borderRadius: 1.5, background: c }} />)}
    </span>);
  if (type === 'profile') return <span style={{ width: 13, height: 13, borderRadius: '50%', border: `1.8px solid ${c}` }} />;
  return <span style={{ width: 13, height: 13, borderRadius: '50%', background: c }} />; // controle (sun)
}
function TabBar({ active = 0 }) {
  const tabs = [['Controle', 'sun'], ['Dispositivos', 'devices'], ['Perfil', 'profile']];
  return (
    <div style={{ display: 'flex', borderTop: `1px solid ${WF.hair}`, padding: '9px 0 13px', flex: '0 0 auto', background: WF.surface }}>
      {tabs.map(([t, g], i) => (
        <div key={t} style={{ flex: 1, display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 5,
          color: i === active ? WF.accentInk : WF.ink2 }}>
          <TabIcon type={g} c={i === active ? WF.accent : WF.ink3} />
          <span style={{ fontSize: 9.5, fontWeight: i === active ? 700 : 500 }}>{t}</span>
        </div>
      ))}
    </div>
  );
}

// section label inside a screen
function Sub({ children, action }) {
  return (
    <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', width: '100%' }}>
      <span style={{ fontSize: 11, color: WF.ink2, fontWeight: 700, ...LBL }}>{children}</span>
      {action && <span style={{ fontSize: 13, color: WF.accentInk, fontWeight: 600 }}>{action}</span>}
    </div>
  );
}

// settings row
function Row({ title, value, icon, chev = true, danger = false }) {
  return (
    <div style={{ display: 'flex', alignItems: 'center', gap: 13, padding: '13px 2px', borderBottom: `1px solid ${WF.hair2}` }}>
      {icon && <div style={{ width: 34, height: 34, borderRadius: 11, background: danger ? '#F6E7E2' : WF.fill, display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
        <span style={{ width: 7, height: 7, borderRadius: '50%', background: danger ? '#B4543A' : WF.ink2 }} /></div>}
      <div style={{ flex: 1, minWidth: 0 }}>
        <div style={{ fontSize: 14.5, fontWeight: 600, color: danger ? '#B4543A' : WF.ink, lineHeight: 1.2 }}>{title}</div>
      </div>
      {value && <span style={{ fontSize: 12, color: WF.ink2, fontWeight: 500, ...NUM }}>{value}</span>}
      {chev && !danger && <Glyph type="chev" size={15} c={WF.ink3} />}
    </div>
  );
}

// elegant canvas annotation (replaces the cursive post-it)
function Note({ children, top, left, right, bottom, width = 168 }) {
  return (
    <div style={{ position: 'absolute', top, left, right, bottom, width,
      background: WF.surface, border: `1px solid ${WF.hair}`, borderRadius: 14, padding: '12px 14px',
      boxShadow: WF.shadowLg, fontFamily: WF.sans, zIndex: 5 }}>
      <div style={{ fontSize: 9.5, color: WF.accentInk, fontWeight: 700, marginBottom: 5, ...LBL }}>fluxo</div>
      <div style={{ fontSize: 12.5, lineHeight: 1.45, color: WF.ink2, fontWeight: 500 }}>{children}</div>
    </div>
  );
}

Object.assign(window, {
  WF, NUM, LBL, Screen, StatusBar, AppBar, IconBtn, Glyph, Box, Img, Lines, Field, Btn, Chip, Dot,
  Segmented, Slider, Fader, Dial, DayChart, Stat, TabBar, Sub, Row, Note,
});
