// screens-auth-control.jsx — Login (2) + Control (3) wireframe screens.
const { WF, Screen, AppBar, IconBtn, Glyph, Box, Img, Lines, Field, Btn, Chip, Dot,
  Segmented, Slider, Fader, Dial, DayChart, Stat, TabBar, Sub } = window;

const pad = { padding: '0 16px' };

// LOGO lockup (square mark + wordmark) — primitive shapes only
function Logo({ size = 44, center }) {
  return (
    <div style={{ display: 'flex', alignItems: 'center', gap: 10, justifyContent: center ? 'center' : 'flex-start' }}>
      <div style={{ width: size, height: size, borderRadius: size * 0.3, background: `linear-gradient(145deg, ${WF.accent}, #C57F1E)`,
        display: 'flex', alignItems: 'center', justifyContent: 'center', boxShadow: '0 6px 18px rgba(217,148,43,.38)' }}>
        <div style={{ width: size * 0.42, height: size * 0.42, borderRadius: '50%', background: 'rgba(255,255,255,.95)', boxShadow: '0 0 0 4px rgba(255,255,255,.3)' }} />
      </div>
      <div style={{ lineHeight: 1.1 }}>
        <div style={{ fontSize: size * 0.46, fontWeight: 800, letterSpacing: '-0.02em' }}>SmartLight</div>
        <div style={{ fontSize: 11.5, color: WF.ink2, fontWeight: 500, marginTop: 2 }}>luz que se ajusta</div>
      </div>
    </div>
  );
}

// ── LOGIN A · Minimal centrado ─────────────────────────────────
function LoginA() {
  return (
    <Screen label="Login A" pad="0">
      <div style={{ flex: 1, display: 'flex', flexDirection: 'column', justifyContent: 'center', gap: 22, ...pad }}>
        <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 8 }}>
          <Logo size={56} center />
          <span style={{ fontSize: 13.5, color: WF.ink2, textAlign: 'center', fontWeight: 500 }}>controle seu interruptor inteligente</span>
        </div>
        <div style={{ display: 'flex', flexDirection: 'column', gap: 14 }}>
          <Field label="E-mail" placeholder="voce@email.com" icon="dot" />
          <Field label="Senha" placeholder="••••••••" icon="dot" />
          <span style={{ fontSize: 13, color: WF.accentInk, alignSelf: 'flex-end', fontWeight: 600 }}>esqueci a senha</span>
        </div>
        <div style={{ display: 'flex', flexDirection: 'column', gap: 10 }}>
          <Btn primary full>Entrar</Btn>
          <Btn ghost full>Entrar como demonstração</Btn>
        </div>
        <span style={{ textAlign: 'center', fontSize: 13.5, color: WF.ink2, fontWeight: 500 }}>
          novo aqui? <span style={{ color: WF.accentInk, fontWeight: 700 }}>criar conta</span></span>
      </div>
    </Screen>
  );
}

// ── LOGIN B · Hero ─────────────────────────────────────────────
function LoginB() {
  return (
    <Screen label="Login B" pad="0">
      <div style={{ ...pad, paddingTop: 4 }}>
        <Img h={172} label="hero: ambiente iluminado" radius={20} />
      </div>
      <div style={{ flex: 1, display: 'flex', flexDirection: 'column', gap: 15, ...pad, paddingTop: 20 }}>
        <div>
          <div style={{ fontSize: 25, lineHeight: 1.1, fontWeight: 800, letterSpacing: '-0.02em' }}>Bem-vindo de volta</div>
          <span style={{ fontSize: 13.5, color: WF.ink2, fontWeight: 500 }}>entre para ajustar suas luzes</span>
        </div>
        <Field placeholder="E-mail" icon="dot" />
        <Field placeholder="Senha" icon="dot" />
        <Btn primary full>Entrar</Btn>
        <div style={{ display: 'flex', alignItems: 'center', gap: 10, color: WF.ink2 }}>
          <div style={{ flex: 1, height: 1, background: WF.hair }} /><span style={{ fontSize: 10, color: WF.ink2, fontWeight: 700, ...LBL }}>ou</span><div style={{ flex: 1, height: 1, background: WF.hair }} />
        </div>
        <Btn ghost full>Continuar com Google</Btn>
        <span style={{ textAlign: 'center', fontSize: 13.5, color: WF.ink2, fontWeight: 500 }}>
          sem conta? <span style={{ color: WF.accentInk, fontWeight: 700 }}>cadastre-se</span></span>
      </div>
    </Screen>
  );
}

// shared device header for the control screens
function DeviceHead({ online = true, sub = 'Sala de estar' }) {
  return (
    <AppBar
      title="Mesa de trabalho"
      sub={sub}
      left={<IconBtn><Glyph type="back" size={17} c={WF.ink} /></IconBtn>}
      right={<IconBtn><Glyph type="more" c={WF.ink2} /></IconBtn>}
    />
  );
}

// ── CONTROL A · Dial-centric ───────────────────────────────────
function ControlA() {
  return (
    <Screen label="Controle A — Dial">
      <DeviceHead />
      <div style={{ ...pad, display: 'flex', justifyContent: 'center', marginBottom: 8 }}>
        <Chip tone="ok"><Dot tone="ok" /> ONLINE</Chip>
      </div>
      <div style={{ ...pad, marginBottom: 14 }}>
        <Segmented options={['Manual', 'Automático']} active={0} />
      </div>
      <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 4, marginBottom: 12 }}>
        <Dial size={184} pct={80} big="320" unit="lux medido" cap="setpoint · 400 lux" />
        <span style={{ fontSize: 12.5, color: WF.ink2, fontWeight: 500, marginTop: 2 }}>luz natural cobrindo <span style={{ color: WF.coolInk, fontWeight: 700 }}>45%</span> do alvo</span>
      </div>
      <div style={{ ...pad, marginBottom: 14 }}>
        <Slider pct={62} label="DIMMER DA LÂMPADA" />
      </div>
      <div style={{ ...pad, display: 'flex', gap: 8, marginBottom: 14 }}>
        <Stat value="180" unit="lux" label="luz natural" tone="cool" />
        <Stat value="62" unit="%" label="dimmer" tone="accent" />
        <Stat value="8.4" unit="W" label="consumo" />
      </div>
      <div style={{ ...pad, marginBottom: 12 }}>
        <DayChart h={84} />
      </div>
      <div style={{ flex: 1 }} />
      <TabBar active={0} />
    </Screen>
  );
}

// ── CONTROL B · Card stack ─────────────────────────────────────
function ControlB() {
  const card = { borderRadius: 18, border: `1px solid ${WF.hair}`, background: WF.surface, padding: 15, boxShadow: WF.shadow };
  return (
    <Screen label="Controle B — Cards">
      <DeviceHead />
      <div style={{ ...pad, display: 'flex', flexDirection: 'column', gap: 12, paddingTop: 4 }}>
        <Segmented options={['Manual', 'Automático']} active={0} />

        {/* hero status card */}
        <div style={{ ...card, background: `linear-gradient(160deg, ${WF.accentSoft}, ${WF.accentSoft2})`, border: `1px solid ${WF.accentSoft}`, boxShadow: '0 8px 24px rgba(217,148,43,.14)' }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start' }}>
            <div>
              <div style={{ fontSize: 10, color: WF.accentInk, ...LBL }}>Luminosidade atual</div>
              <div style={{ fontSize: 38, fontWeight: 800, color: WF.ink, lineHeight: 1, marginTop: 3, ...NUM }}>320<span style={{ fontSize: 14, fontWeight: 600, color: WF.accentInk }}> lux</span></div>
            </div>
            <Chip tone="ok"><Dot tone="ok" /> ONLINE</Chip>
          </div>
          <div style={{ marginTop: 10 }}><Slider pct={80} /></div>
          <div style={{ fontSize: 10, color: WF.accentInk, marginTop: 10, ...LBL, letterSpacing: '0.04em' }}>setpoint configurado · 400 lux</div>
        </div>

        {/* dimmer card */}
        <div style={card}>
          <Slider pct={62} label="DIMMER DA LÂMPADA" />
          <div style={{ display: 'flex', justifyContent: 'space-between', marginTop: 12, fontSize: 11.5, color: WF.ink2, fontWeight: 500 }}>
            <span>natural · 180 lux</span><span>consumo · 8,4 W</span>
          </div>
        </div>

        {/* chart card */}
        <div style={card}>
          <DayChart h={78} />
        </div>
      </div>
      <div style={{ flex: 1 }} />
      <TabBar active={0} />
    </Screen>
  );
}

// ── CONTROL C · Vertical fader ─────────────────────────────────
function ControlC() {
  return (
    <Screen label="Controle C — Fader">
      <DeviceHead />
      <div style={{ ...pad, display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 12 }}>
        <Chip tone="ok"><Dot tone="ok" /> ONLINE</Chip>
        <Segmented options={['Man.', 'Auto']} active={0} style={{ width: 120 }} />
      </div>
      <div style={{ ...pad, display: 'flex', gap: 16, marginBottom: 14 }}>
        <Fader natural={45} lamp={30} setpoint={75} h={232} />
        <div style={{ flex: 1, display: 'flex', flexDirection: 'column', gap: 10, paddingTop: 4 }}>
          <div>
            <div style={{ fontSize: 10, color: WF.ink2, ...LBL }}>Total medido</div>
            <div style={{ fontSize: 30, fontWeight: 800, lineHeight: 1, marginTop: 3, ...NUM }}>320<span style={{ fontSize: 12, fontWeight: 600, color: WF.ink2 }}> lux</span></div>
          </div>
          <div style={{ height: 1, background: WF.hair }} />
          <div style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
            <div style={{ width: 14, height: 14, borderRadius: 4, background: WF.coolSoft, border: `1px solid ${WF.cool}` }} />
            <span style={{ fontSize: 11.5, color: WF.ink2, fontWeight: 500 }}>natural · 180 lux</span>
          </div>
          <div style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
            <div style={{ width: 14, height: 14, borderRadius: 4, background: WF.accentSoft, border: `1px solid ${WF.accent}` }} />
            <span style={{ fontSize: 11.5, color: WF.ink2, fontWeight: 500 }}>lâmpada · 140 lux</span>
          </div>
          <div style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
            <div style={{ width: 14, height: 2, borderTop: `2px dashed ${WF.accentInk}` }} />
            <span style={{ fontSize: 11.5, color: WF.accentInk, fontWeight: 600 }}>setpoint · 400 lux</span>
          </div>
          <div style={{ marginTop: 2, fontSize: 12.5, color: WF.ink2, fontWeight: 500, lineHeight: 1.35 }}>arraste o fader p/ ajustar a lâmpada</div>
        </div>
      </div>
      <div style={{ ...pad, display: 'flex', gap: 8, marginBottom: 12 }}>
        <Stat value="62" unit="%" label="dimmer" tone="accent" />
        <Stat value="8.4" unit="W" label="consumo agora" />
        <Stat value="0.21" unit="kWh" label="hoje" />
      </div>
      <div style={{ flex: 1 }} />
      <TabBar active={0} />
    </Screen>
  );
}

Object.assign(window, { LoginA, LoginB, ControlA, ControlB, ControlC });
