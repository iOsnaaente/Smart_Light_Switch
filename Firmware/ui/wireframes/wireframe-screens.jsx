/* wireframe-screens.jsx — telas do interruptor inteligente em RETRATO (240x320).
   Fonte Manrope (cara de home assistant). Estilo limpo e sério, dirigido por
   variaveis CSS (--accent, --r, --ui, --ink, --paper, --card, --track, --hair, --muted, --natural).
   Exporta componentes para window no final. */

const FONT = "'Manrope', system-ui, -apple-system, sans-serif";
const W = 240, H = 320;

// ───────────────────────────────────────── strings ──────────────────────────
const STR = {
  pt: {
    room: "Sala de Estar", on: "Ligada", off: "Desligada", auto: "AUTO", manual: "MANUAL",
    natural: "Natural", lamp: "Lâmpada", target: "Alvo", ambient: "Ambiente",
    bright: "Intensidade", lux: "lx", autoMode: "Modo Automático", settings: "Ajustes",
    language: "Idioma", screenBright: "Brilho da tela", calib: "Calibrar sensor",
    about: "Sobre", sensitivity: "Sensibilidade", med: "média", compensating: "compensando",
    connected: "conectado", whenDark: "Quando escurece, a lâmpada completa até o alvo."
  },
  en: {
    room: "Living Room", on: "On", off: "Off", auto: "AUTO", manual: "MANUAL",
    natural: "Natural", lamp: "Lamp", target: "Target", ambient: "Ambient",
    bright: "Brightness", lux: "lx", autoMode: "Auto Mode", settings: "Settings",
    language: "Language", screenBright: "Screen brightness", calib: "Calibrate sensor",
    about: "About", sensitivity: "Sensitivity", med: "medium", compensating: "compensating",
    connected: "connected", whenDark: "When it gets dark, the lamp fills up to target."
  }
};

// ───────────────────────────────────────── icons (formas simples) ───────────
function Icon({ name, size = 16, on = true, stroke = "var(--ink)" }) {
  if (!on) return null;
  const s = size;
  const base = { width: s, height: s, position: "relative", display: "inline-block", flex: "none" };
  if (name === "wifi") {
    return (
      <span style={{ ...base, display: "inline-flex", alignItems: "flex-end", gap: s * 0.12 }}>
        {[0.4, 0.65, 0.9].map((h, i) => (
          <i key={i} style={{ width: s * 0.2, height: s * h, background: stroke, borderRadius: 1 }} />
        ))}
      </span>
    );
  }
  if (name === "sun") {
    return (
      <span style={base}>
        <i style={{ position: "absolute", inset: s * 0.16, borderRadius: "50%", border: `1.5px solid ${stroke}` }} />
        <i style={{ position: "absolute", inset: s * 0.34, borderRadius: "50%", background: stroke }} />
      </span>
    );
  }
  if (name === "bulb") {
    return (
      <span style={base}>
        <i style={{ position: "absolute", left: s * 0.18, top: s * 0.04, width: s * 0.64, height: s * 0.64, borderRadius: "50%", border: `1.5px solid ${stroke}` }} />
        <i style={{ position: "absolute", left: s * 0.34, bottom: s * 0.02, width: s * 0.32, height: s * 0.16, background: stroke, borderRadius: 2 }} />
      </span>
    );
  }
  if (name === "power") {
    return (
      <span style={base}>
        <i style={{ position: "absolute", inset: s * 0.12, borderRadius: "50%", border: `2px solid ${stroke}`, clipPath: "polygon(0 18%, 100% 18%, 100% 100%, 0 100%)" }} />
        <i style={{ position: "absolute", left: "50%", top: s * 0.04, width: 2, height: s * 0.44, background: stroke, transform: "translateX(-50%)", borderRadius: 2 }} />
      </span>
    );
  }
  if (name === "gear") {
    return (
      <span style={{ ...base, display: "inline-flex", flexDirection: "column", justifyContent: "center", gap: s * 0.18 }}>
        {[0.25, 0.6, 0.4].map((p, i) => (
          <span key={i} style={{ position: "relative", height: 2, background: stroke, borderRadius: 2 }}>
            <i style={{ position: "absolute", left: `${p * 100}%`, top: "50%", width: s * 0.16, height: s * 0.16, borderRadius: "50%", background: "var(--card)", border: `1.5px solid ${stroke}`, transform: "translate(-50%,-50%)" }} />
          </span>
        ))}
      </span>
    );
  }
  return null;
}

function Chevron({ dir = "right", size = 9, color = "var(--ink)", weight = 2 }) {
  const rot = { right: 45, left: 225, up: 315, down: 135 }[dir];
  return (
    <i style={{
      width: size, height: size, display: "inline-block", flex: "none",
      borderTop: `${weight}px solid ${color}`, borderRight: `${weight}px solid ${color}`,
      transform: `rotate(${rot}deg)`
    }} />
  );
}

// ───────────────────────────────────────── status bar ───────────────────────
function StatusBar({ s, icons, online = true, mode = "auto" }) {
  return (
    <div style={{
      display: "flex", alignItems: "center", justifyContent: "space-between",
      padding: "0 14px", height: 30, flex: "none",
      borderBottom: "1px solid var(--hair)", color: "var(--muted)"
    }}>
      <div style={{ display: "flex", alignItems: "center", gap: 6, fontSize: "calc(9.5px*var(--ui))", letterSpacing: ".02em" }}>
        <Icon name="wifi" size={12} on={icons} stroke="var(--muted)" />
        <span>{online ? s.connected : "—"}</span>
      </div>
      <div style={{
        display: "flex", alignItems: "center", gap: 5, fontSize: "calc(9.5px*var(--ui))",
        fontWeight: 700, letterSpacing: ".1em",
        color: mode === "auto" ? "var(--accent)" : "var(--muted)"
      }}>
        {mode === "auto" && <i style={{ width: 6, height: 6, borderRadius: "50%", background: "var(--accent)" }} />}
        {mode === "auto" ? s.auto : s.manual}
      </div>
    </div>
  );
}

// ───────────────────────────────────────── intensity controls ───────────────
function Dial({ value, size = 130 }) {
  return (
    <div style={{ width: size, height: size, position: "relative", flex: "none" }}>
      <div style={{
        position: "absolute", inset: 0, borderRadius: "50%",
        background: `conic-gradient(var(--accent) ${value * 3.6}deg, var(--track) 0)`
      }} />
      <div style={{
        position: "absolute", inset: size * 0.14, borderRadius: "50%", background: "var(--card)",
        boxShadow: "inset 0 0 0 1px var(--hair)",
        display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center"
      }}>
        <div style={{ fontWeight: 800, fontSize: `calc(${size * 0.3}px*var(--ui))`, lineHeight: 0.9, color: "var(--ink)", letterSpacing: "-.02em" }}>
          {value}<span style={{ fontSize: `calc(${size * 0.15}px*var(--ui))`, fontWeight: 700 }}>%</span>
        </div>
        <div style={{ fontSize: "calc(8.5px*var(--ui))", color: "var(--muted)", textTransform: "uppercase", letterSpacing: ".08em", marginTop: 2 }}>brilho</div>
      </div>
    </div>
  );
}

function VSlider({ value, height = 130 }) {
  return (
    <div style={{ display: "flex", alignItems: "flex-end", gap: 12, flex: "none" }}>
      <div style={{
        width: 44, height, borderRadius: "var(--r)", boxShadow: "inset 0 0 0 1px var(--hair)",
        background: "var(--track)", position: "relative", overflow: "hidden"
      }}>
        <div style={{ position: "absolute", left: 0, right: 0, bottom: 0, height: `${value}%`, background: "var(--accent)" }} />
      </div>
      <div style={{ fontWeight: 800, fontSize: "calc(32px*var(--ui))", color: "var(--ink)", lineHeight: 1, letterSpacing: "-.02em" }}>
        {value}<span style={{ fontSize: "calc(15px*var(--ui))", fontWeight: 700 }}>%</span>
      </div>
    </div>
  );
}

function Stepper({ value }) {
  const btn = {
    width: 44, height: 44, borderRadius: "var(--r)", boxShadow: "inset 0 0 0 1.5px var(--ink)",
    background: "var(--card)", display: "flex", alignItems: "center", justifyContent: "center",
    fontSize: "calc(24px*var(--ui))", fontWeight: 600, color: "var(--ink)", flex: "none"
  };
  return (
    <div style={{ display: "flex", alignItems: "center", gap: 12, flex: "none" }}>
      <div style={btn}>−</div>
      <div style={{ fontWeight: 800, fontSize: "calc(40px*var(--ui))", color: "var(--ink)", lineHeight: 0.9, minWidth: 60, textAlign: "center", letterSpacing: "-.02em" }}>{value}<span style={{ fontSize: "calc(18px*var(--ui))", fontWeight: 700 }}>%</span></div>
      <div style={{ ...btn, background: "var(--accent)", boxShadow: "none", color: "#fff" }}>+</div>
    </div>
  );
}

function Intensity({ variant, value, size }) {
  if (variant === "slider") return <VSlider value={value} height={size} />;
  if (variant === "stepper") return <Stepper value={value} />;
  return <Dial value={value} size={size} />;
}

function PowerBtn({ s, on, icons }) {
  return (
    <div style={{
      height: 46, borderRadius: "var(--r)", display: "flex", alignItems: "center",
      justifyContent: "center", gap: 8, flex: "none",
      background: on ? "var(--accent)" : "var(--card)",
      boxShadow: on ? "none" : "inset 0 0 0 1.5px var(--ink)",
      color: on ? "#fff" : "var(--ink)", fontWeight: 700, letterSpacing: ".01em",
      fontSize: "calc(15px*var(--ui))"
    }}>
      <Icon name="power" size={18} on={icons} stroke={on ? "#fff" : "var(--ink)"} />
      {on ? s.on : s.off}
    </div>
  );
}

function Card({ children, style, pad = 10 }) {
  return (
    <div style={{
      boxShadow: "inset 0 0 0 1px var(--hair)", borderRadius: "var(--r)", background: "var(--card)",
      padding: pad, ...style
    }}>{children}</div>
  );
}

function Btn({ children, accent }) {
  return (
    <div style={{
      width: 42, height: 42, borderRadius: "var(--r)", display: "flex", alignItems: "center", justifyContent: "center",
      boxShadow: accent ? "none" : "inset 0 0 0 1.5px var(--ink)",
      background: accent ? "var(--accent)" : "var(--card)", flex: "none"
    }}>{children}</div>
  );
}

function Label({ children, style }) {
  return <div style={{ fontSize: "calc(9px*var(--ui))", color: "var(--muted)", textTransform: "uppercase", letterSpacing: ".07em", fontWeight: 600, whiteSpace: "nowrap", ...style }}>{children}</div>;
}

// ───────────────────────────────────────── Screen shell (240x320) ───────────
function Screen({ children }) {
  return (
    <div className="wf-screen" style={{
      width: W, height: H, background: "var(--paper)", color: "var(--ink)",
      fontFamily: FONT, display: "flex", flexDirection: "column", overflow: "hidden"
    }}>{children}</div>
  );
}

// ═══════════════════════════ PRINCIPAL — A · Dial central ════════════════════
function MainA({ lang, icons, control }) {
  const s = STR[lang];
  const stat = (icon, label, val, unit) => (
    <Card style={{ flex: 1 }} pad={9}>
      <div style={{ display: "flex", alignItems: "center", gap: 5 }}>
        <Icon name={icon} size={12} on={icons} stroke="var(--muted)" /><Label>{label}</Label>
      </div>
      <div style={{ fontWeight: 800, fontSize: "calc(17px*var(--ui))", marginTop: 3, letterSpacing: "-.01em" }}>{val}<span style={{ fontSize: "calc(10px*var(--ui))", color: "var(--muted)", fontWeight: 600 }}> {unit}</span></div>
    </Card>
  );
  return (
    <Screen>
      <StatusBar s={s} icons={icons} mode="auto" />
      <div style={{ flex: 1, display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", padding: "0 16px", gap: 14 }}>
        <Label style={{ fontSize: "calc(11px*var(--ui))", letterSpacing: ".06em" }}>{s.room}</Label>
        <Intensity variant={control} value={72} size={130} />
        <div style={{ display: "flex", gap: 10, width: "100%" }}>
          {stat("sun", s.natural, 340, s.lux)}
          {stat("bulb", s.ambient, 520, s.lux)}
        </div>
      </div>
      <div style={{ padding: "0 16px 16px" }}><PowerBtn s={s} on={true} icons={icons} /></div>
    </Screen>
  );
}

// ═══════════════════════════ PRINCIPAL — B · Sol vs Lâmpada ══════════════════
function MainB({ lang, icons }) {
  const s = STR[lang];
  const naturalPct = 56, lampPct = 24;
  return (
    <Screen>
      <StatusBar s={s} icons={icons} mode="auto" />
      <div style={{ flex: 1, padding: "14px 14px 0", display: "flex", flexDirection: "column", gap: 12 }}>
        <div style={{ display: "flex", justifyContent: "space-between", alignItems: "baseline" }}>
          <span style={{ fontSize: "calc(14px*var(--ui))", fontWeight: 700, whiteSpace: "nowrap" }}>{s.room}</span>
          <span style={{ fontSize: "calc(10px*var(--ui))", color: "var(--accent)", fontWeight: 700, whiteSpace: "nowrap" }}>{s.compensating}…</span>
        </div>
        <div>
          <div style={{ position: "relative", height: 30, borderRadius: "var(--r)", overflow: "hidden", boxShadow: "inset 0 0 0 1px var(--hair)", background: "var(--track)", display: "flex" }}>
            <div style={{ width: `${naturalPct}%`, background: "var(--natural)", display: "flex", alignItems: "center", paddingLeft: 8 }}>{icons && <Icon name="sun" size={13} stroke="var(--ink)" />}</div>
            <div style={{ width: `${lampPct}%`, background: "var(--accent)" }} />
            <div style={{ position: "absolute", left: `${naturalPct + lampPct}%`, top: -2, bottom: -2, width: 0, borderLeft: "2px dashed var(--ink)" }} />
          </div>
          <div style={{ display: "flex", justifyContent: "space-between", marginTop: 6, fontSize: "calc(9.5px*var(--ui))", color: "var(--muted)", fontWeight: 600 }}>
            <span>{s.natural} 340</span>
            <span style={{ color: "var(--accent)" }}>+ {s.lamp} 36%</span>
            <span style={{ color: "var(--ink)" }}>{s.target} 500</span>
          </div>
        </div>
        <Card style={{ display: "flex", alignItems: "center", justifyContent: "space-between", gap: 6 }} pad={8}>
          <Label>{s.target} ({s.lux})</Label>
          <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
            <Btn><Chevron dir="down" /></Btn>
            <div style={{ fontWeight: 800, fontSize: "calc(26px*var(--ui))", lineHeight: 1, minWidth: 50, textAlign: "center", letterSpacing: "-.02em" }}>500</div>
            <Btn accent><Chevron dir="up" color="#fff" /></Btn>
          </div>
        </Card>
      </div>
      <div style={{ padding: "12px 14px 16px" }}><PowerBtn s={s} on={true} icons={icons} /></div>
    </Screen>
  );
}

// ═══════════════════════════ PRINCIPAL — C · Dashboard cards ═════════════════
function MainC({ lang, icons, control }) {
  const s = STR[lang];
  const tile = (icon, label, val) => (
    <Card style={{ flex: 1, display: "flex", flexDirection: "column", gap: 4 }} pad={9}>
      <div style={{ display: "flex", alignItems: "center", gap: 5 }}><Icon name={icon} size={12} on={icons} stroke="var(--muted)" /><Label>{label}</Label></div>
      <div style={{ fontWeight: 800, fontSize: "calc(18px*var(--ui))", lineHeight: 1, letterSpacing: "-.01em" }}>{val}<span style={{ fontSize: "calc(10px*var(--ui))", color: "var(--muted)", fontWeight: 600 }}> {s.lux}</span></div>
    </Card>
  );
  return (
    <Screen>
      <StatusBar s={s} icons={icons} mode="auto" />
      <div style={{ flex: 1, padding: 12, display: "flex", flexDirection: "column", gap: 9 }}>
        <Card style={{ flex: 1, display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", gap: 8, position: "relative" }} pad={10}>
          <Label style={{ position: "absolute", top: 10, left: 12 }}>{s.room}</Label>
          <Intensity variant={control} value={72} size={control === "dial" ? 104 : 96} />
        </Card>
        <div style={{ display: "flex", gap: 9 }}>
          {tile("sun", s.natural, 340)}
          {tile("bulb", s.target, 500)}
        </div>
        <PowerBtn s={s} on={true} icons={icons} />
      </div>
    </Screen>
  );
}

// ═══════════════════════════ MODO AUTOMÁTICO ═════════════════════════════════
function AutoScreen({ lang, icons }) {
  const s = STR[lang];
  return (
    <Screen>
      <div style={{ display: "flex", alignItems: "center", gap: 10, padding: "0 14px", height: 42, flex: "none", borderBottom: "1px solid var(--hair)" }}>
        <Chevron dir="left" size={9} />
        <span style={{ fontSize: "calc(14px*var(--ui))", fontWeight: 700 }}>{s.autoMode}</span>
        <div style={{ marginLeft: "auto", width: 44, height: 25, borderRadius: 13, background: "var(--accent)", position: "relative", flex: "none" }}>
          <i style={{ position: "absolute", top: 3, right: 3, width: 19, height: 19, borderRadius: "50%", background: "#fff" }} />
        </div>
      </div>
      <div style={{ flex: 1, padding: "12px 14px", display: "flex", flexDirection: "column", gap: 11 }}>
        <div style={{ fontSize: "calc(11px*var(--ui))", color: "var(--muted)", lineHeight: 1.35 }}>{s.whenDark}</div>
        <Card style={{ display: "flex", alignItems: "center", justifyContent: "space-between" }} pad={10}>
          <div>
            <Label>{s.target}</Label>
            <div style={{ fontWeight: 800, fontSize: "calc(30px*var(--ui))", lineHeight: 0.95, letterSpacing: "-.02em" }}>500<span style={{ fontSize: "calc(12px*var(--ui))", color: "var(--muted)", fontWeight: 600 }}> {s.lux}</span></div>
          </div>
          <div style={{ display: "flex", gap: 9 }}>
            <Btn><Chevron dir="down" /></Btn>
            <Btn accent><Chevron dir="up" color="#fff" /></Btn>
          </div>
        </Card>
        <div>
          <Label style={{ marginBottom: 6 }}>{s.natural} + {s.lamp}</Label>
          <div style={{ display: "flex", height: 22, borderRadius: "var(--r)", overflow: "hidden", boxShadow: "inset 0 0 0 1px var(--hair)" }}>
            <div style={{ width: "56%", background: "var(--natural)" }} />
            <div style={{ width: "24%", background: "var(--accent)" }} />
            <div style={{ flex: 1, background: "var(--track)" }} />
          </div>
        </div>
        <div>
          <div style={{ display: "flex", justifyContent: "space-between", alignItems: "baseline" }}>
            <Label>{s.sensitivity}</Label><span style={{ fontSize: "calc(10px*var(--ui))", color: "var(--ink)", fontWeight: 600 }}>{s.med}</span>
          </div>
          <div style={{ marginTop: 8, height: 6, borderRadius: 3, background: "var(--track)", position: "relative" }}>
            <i style={{ position: "absolute", left: 0, top: 0, bottom: 0, width: "55%", background: "var(--accent)", borderRadius: 3 }} />
            <i style={{ position: "absolute", left: "55%", top: "50%", width: 18, height: 18, borderRadius: "50%", background: "var(--card)", boxShadow: "0 1px 4px rgba(0,0,0,.2), inset 0 0 0 1.5px var(--accent)", transform: "translate(-50%,-50%)" }} />
          </div>
        </div>
      </div>
    </Screen>
  );
}

// ═══════════════════════════ AJUSTES ═════════════════════════════════════════
function SettingsScreen({ lang, icons }) {
  const s = STR[lang];
  const Row = ({ icon, label, right, last }) => (
    <div style={{ display: "flex", alignItems: "center", gap: 11, height: 46, borderBottom: last ? "none" : "1px solid var(--hair)" }}>
      {icon && <Icon name={icon} size={16} on={icons} stroke="var(--muted)" />}
      <span style={{ fontSize: "calc(12.5px*var(--ui))", fontWeight: 500 }}>{label}</span>
      <span style={{ marginLeft: "auto", display: "flex", alignItems: "center", gap: 7, fontSize: "calc(11px*var(--ui))", color: "var(--muted)", fontWeight: 600 }}>{right}</span>
    </div>
  );
  const Seg = ({ a, b, active }) => (
    <div style={{ display: "flex", boxShadow: "inset 0 0 0 1.5px var(--ink)", borderRadius: "var(--r)", overflow: "hidden", fontSize: "calc(11px*var(--ui))", fontWeight: 700 }}>
      <span style={{ padding: "4px 11px", background: active === "a" ? "var(--accent)" : "transparent", color: active === "a" ? "#fff" : "var(--ink)" }}>{a}</span>
      <span style={{ padding: "4px 11px", background: active === "b" ? "var(--accent)" : "transparent", color: active === "b" ? "#fff" : "var(--ink)" }}>{b}</span>
    </div>
  );
  return (
    <Screen>
      <div style={{ display: "flex", alignItems: "center", gap: 10, padding: "0 14px", height: 42, flex: "none", borderBottom: "1px solid var(--hair)" }}>
        <Chevron dir="left" size={9} />
        <span style={{ fontSize: "calc(14px*var(--ui))", fontWeight: 700 }}>{s.settings}</span>
        <span style={{ marginLeft: "auto" }}><Icon name="gear" size={16} on={icons} stroke="var(--muted)" /></span>
      </div>
      <div style={{ flex: 1, padding: "2px 14px" }}>
        <Row icon="wifi" label={s.language} right={<Seg a="PT" b="EN" active={lang === "pt" ? "a" : "b"} />} />
        <Row icon="sun" label={s.screenBright} right={
          <span style={{ width: 58, height: 6, borderRadius: 3, background: "var(--track)", position: "relative", display: "inline-block" }}>
            <i style={{ position: "absolute", left: 0, top: 0, bottom: 0, width: "70%", background: "var(--accent)", borderRadius: 3 }} />
          </span>
        } />
        <Row icon="bulb" label={s.calib} right={<Chevron dir="right" size={8} color="var(--muted)" />} />
        <Row icon="wifi" label="Wi-Fi" right={<><span>casa-2G</span><Chevron dir="right" size={8} color="var(--muted)" /></>} />
        <Row icon="gear" label={s.about} right={<span>v1.0.3</span>} last />
      </div>
    </Screen>
  );
}

// ───────────────────────────────────────── device frame ─────────────────────
function Device({ label, tag, children, scale = 1.5 }) {
  return (
    <div style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 12 }}>
      <div style={{
        background: "#1f2227", borderRadius: 22, padding: 13,
        boxShadow: "0 12px 34px rgba(0,0,0,.18), inset 0 1px 0 rgba(255,255,255,.06)",
        position: "relative"
      }}>
        <i style={{ position: "absolute", left: 7, top: "50%", transform: "translateY(-50%)", width: 3, height: 26, borderRadius: 2, background: "rgba(255,255,255,.16)" }} />
        <div style={{ width: W * scale, height: H * scale, borderRadius: 8, overflow: "hidden", boxShadow: "inset 0 0 0 1px rgba(0,0,0,.45)" }}>
          <div style={{ width: W, height: H, transformOrigin: "top left", transform: `scale(${scale})` }}>{children}</div>
        </div>
      </div>
      <div style={{ textAlign: "center", fontFamily: FONT }}>
        {tag && <div style={{ fontWeight: 800, fontSize: 13, color: "var(--accent)", letterSpacing: ".02em" }}>{tag}</div>}
        <div style={{ fontSize: 13, color: "#6b6f76", fontWeight: 600 }}>{label}</div>
      </div>
    </div>
  );
}

Object.assign(window, {
  STR, FONT, Icon, Chevron, StatusBar, Dial, VSlider, Stepper, Intensity, PowerBtn, Card, Btn, Label,
  Screen, MainA, MainB, MainC, AutoScreen, SettingsScreen, Device
});
