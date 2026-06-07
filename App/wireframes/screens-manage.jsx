// screens-manage.jsx — Switch list (2) + Device config (2) + States (2).
const { WF, NUM, LBL, Screen, AppBar, IconBtn, Glyph, Box, Img, Lines, Field, Btn, Chip, Dot,
  Segmented, Slider, Fader, Dial, DayChart, Stat, TabBar, Sub, Row } = window;

const padM = { padding: '0 16px' };

// tiny on/off switch
function Toggle({ on }) {
  return (
    <div style={{ width: 42, height: 24, borderRadius: 14, background: on ? WF.accent : WF.fill,
      position: 'relative', flex: '0 0 auto', boxShadow: on ? 'inset 0 0 0 1px rgba(154,100,18,.18)' : `inset 0 0 0 1px ${WF.hair}` }}>
      <div style={{ position: 'absolute', top: 2.5, left: on ? 20 : 2.5, width: 19, height: 19, borderRadius: '50%',
        background: '#fff', boxShadow: '0 1px 3px rgba(35,30,22,.25)' }} />
    </div>
  );
}

// mini circular gauge for grid cards
function MiniDial({ pct = 70, big = '320' }) {
  const s = 64, r = 24, C = 2 * Math.PI * r, a = 0.75;
  return (
    <div style={{ position: 'relative', width: s, height: s }}>
      <svg width={s} height={s}>
        <circle cx={s/2} cy={s/2} r={r} fill="none" stroke={WF.hair} strokeWidth="6" strokeDasharray={`${C*a} ${C}`} strokeLinecap="round" transform={`rotate(135 ${s/2} ${s/2})`} />
        {pct > 0 && <circle cx={s/2} cy={s/2} r={r} fill="none" stroke={WF.accent} strokeWidth="6" strokeDasharray={`${C*a*(pct/100)} ${C}`} strokeLinecap="round" transform={`rotate(135 ${s/2} ${s/2})`} />}
      </svg>
      <div style={{ position: 'absolute', inset: 0, display: 'flex', alignItems: 'center', justifyContent: 'center', fontSize: 13, fontWeight: 800, ...NUM }}>{big}</div>
    </div>
  );
}

// list row for variation A
function SwitchRow({ name, mode, lux, dim, on = true, status = 'ok' }) {
  return (
    <div style={{ display: 'flex', alignItems: 'center', gap: 12, padding: '12px 2px', borderBottom: `1px solid ${WF.hair2}` }}>
      <MiniDial pct={dim} big={lux} />
      <div style={{ flex: 1, minWidth: 0 }}>
        <div style={{ fontSize: 15.5, fontWeight: 600, lineHeight: 1.1 }}>{name}</div>
        <div style={{ display: 'flex', gap: 5, marginTop: 4 }}>
          <Chip tone={status === 'off' ? 'off' : 'plain'}><Dot tone={status} /> {status === 'off' ? 'OFFLINE' : 'online'}</Chip>
          <Chip tone={mode === 'Auto' ? 'on' : 'plain'}>{mode}</Chip>
        </div>
      </div>
      <Toggle on={on && status !== 'off'} />
    </div>
  );
}

// ── LIST A · agrupado por ambiente ─────────────────────────────
function ListA() {
  return (
    <Screen label="Dispositivos A — Lista">
      <AppBar title="Meus interruptores" sub="4 dispositivos · 3 online"
        left={<IconBtn><Glyph type="menu" c={WF.ink} /></IconBtn>} right={<IconBtn><Glyph type="plus" size={18} c={WF.accentInk} /></IconBtn>} />
      <div style={{ ...padM }}>
        <Field placeholder="Buscar dispositivo ou ambiente" icon="dot" />
      </div>
      <div style={{ ...padM, marginTop: 14, display: 'flex', flexDirection: 'column', gap: 8 }}>
        <Sub action="ligar todos">Sala de estar</Sub>
        <SwitchRow name="Mesa de trabalho" mode="Manual" lux="320" dim={62} on />
        <SwitchRow name="Luz de leitura" mode="Auto" lux="410" dim={48} on />
      </div>
      <div style={{ ...padM, marginTop: 16, display: 'flex', flexDirection: 'column', gap: 8 }}>
        <Sub action="ligar todos">Quarto</Sub>
        <SwitchRow name="Teto" mode="Auto" lux="280" dim={70} on />
        <SwitchRow name="Closet" mode="Manual" lux="—" dim={0} on={false} status="off" />
      </div>
      <div style={{ flex: 1 }} />
      <TabBar active={1} />
    </Screen>
  );
}

// grid card for variation B
function GridCard({ name, room, lux, dim, mode, status = 'ok' }) {
  return (
    <div style={{ flex: 1, borderRadius: 18, border: `1px solid ${WF.hair}`, boxShadow: WF.shadow,
      background: WF.surface, padding: 13, display: 'flex', flexDirection: 'column', gap: 9,
      opacity: status === 'off' ? 0.6 : 1 }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
        <Dot tone={status} /><Toggle on={status !== 'off'} />
      </div>
      <div style={{ display: 'flex', justifyContent: 'center' }}><MiniDial pct={dim} big={lux} /></div>
      <div>
        <div style={{ fontSize: 14.5, fontWeight: 600, lineHeight: 1.1 }}>{name}</div>
        <div style={{ fontSize: 10.5, color: WF.ink2, fontWeight: 500, marginTop: 1 }}>{room}</div>
      </div>
      <Chip tone={mode === 'Auto' ? 'on' : 'plain'}>{mode}</Chip>
    </div>
  );
}

// ── LIST B · grade de cards ────────────────────────────────────
function ListB() {
  return (
    <Screen label="Dispositivos B — Grade">
      <AppBar title="Interruptores" sub="4 dispositivos"
        left={<IconBtn><Glyph type="menu" c={WF.ink} /></IconBtn>} right={<IconBtn><Glyph type="plus" size={18} c={WF.accentInk} /></IconBtn>} />
      <div style={{ ...padM, display: 'flex', gap: 7, marginBottom: 12, flexWrap: 'wrap' }}>
        <Chip tone="on">Todos</Chip><Chip>Sala</Chip><Chip>Quarto</Chip><Chip>Cozinha</Chip>
      </div>
      <div style={{ ...padM, display: 'flex', flexDirection: 'column', gap: 10 }}>
        <div style={{ display: 'flex', gap: 10 }}>
          <GridCard name="Mesa de trabalho" room="Sala" lux="320" dim={62} mode="Manual" />
          <GridCard name="Luz de leitura" room="Sala" lux="410" dim={48} mode="Auto" />
        </div>
        <div style={{ display: 'flex', gap: 10 }}>
          <GridCard name="Teto" room="Quarto" lux="280" dim={70} mode="Auto" />
          <GridCard name="Closet" room="Quarto" lux="—" dim={0} mode="Manual" status="off" />
        </div>
        <Box h={48} dashed label="+ adicionar interruptor" />
      </div>
      <div style={{ flex: 1 }} />
      <TabBar active={1} />
    </Screen>
  );
}

// ── DETAIL A · lista de configurações ──────────────────────────
function DetailA() {
  return (
    <Screen label="Gerenciar A — Config">
      <AppBar title="Mesa de trabalho" sub="Sala de estar"
        left={<IconBtn><Glyph type="back" size={17} c={WF.ink} /></IconBtn>} right={<IconBtn><Glyph type="more" c={WF.ink2} /></IconBtn>} />
      <div style={{ ...padM }}>
        <div style={{ borderRadius: 18, border: `1px solid ${WF.hair}`, background: WF.surface, padding: 15, boxShadow: WF.shadow,
          display: 'flex', alignItems: 'center', gap: 13 }}>
          <MiniDial pct={62} big="320" />
          <div style={{ flex: 1 }}>
            <Chip tone="ok"><Dot tone="ok" /> ONLINE</Chip>
            <div style={{ fontSize: 11, color: WF.ink2, marginTop: 8, fontWeight: 500, ...NUM }}>firmware v2.1 · −52 dBm</div>
          </div>
        </div>
      </div>
      <div style={{ ...padM, marginTop: 16, display: 'flex', flexDirection: 'column', gap: 4 }}>
        <Sub>Luminosidade</Sub>
        <Row title="Setpoint de luminosidade" value="400 lux" icon="dot" />
        <Row title="Limites do dimmer" value="10–100%" icon="dot" />
      </div>
      <div style={{ ...padM, marginTop: 14, display: 'flex', flexDirection: 'column', gap: 4 }}>
        <Sub>Dispositivo</Sub>
        <Row title="Calibrar sensor de luz" icon="dot" />
        <Row title="Renomear / ambiente" value="Mesa · Sala" icon="dot" />
        <Row title="Agendamentos" value="2 ativos" icon="dot" />
        <Row title="Wi-Fi e conexão" value="CasaWiFi" icon="dot" />
      </div>
      <div style={{ ...padM, marginTop: 14 }}>
        <Row title="Remover dispositivo" icon="close" chev={false} danger />
      </div>
      <div style={{ flex: 1 }} />
    </Screen>
  );
}

// ── DETAIL B · setpoint em destaque ────────────────────────────
function DetailB() {
  return (
    <Screen label="Gerenciar B — Setpoint">
      <AppBar title="Ajustar interruptor" sub="Mesa de trabalho"
        left={<IconBtn><Glyph type="back" size={17} c={WF.ink} /></IconBtn>} right={<span style={{ fontSize: 14, fontWeight: 700, color: WF.accentInk }}>Salvar</span>} />
      {/* setpoint hero */}
      <div style={{ ...padM }}>
        <div style={{ borderRadius: 20, border: `1px solid ${WF.accentSoft}`, background: `linear-gradient(160deg, ${WF.accentSoft}, ${WF.accentSoft2})`, padding: 18, textAlign: 'center', boxShadow: '0 8px 24px rgba(217,148,43,.14)' }}>
          <div style={{ fontSize: 10, color: WF.accentInk, ...LBL }}>Setpoint de luminosidade</div>
          <div style={{ fontSize: 44, fontWeight: 800, lineHeight: 1.1, marginTop: 4, ...NUM }}>400<span style={{ fontSize: 15, fontWeight: 600, color: WF.accentInk }}> lux</span></div>
          <div style={{ marginTop: 12 }}><Slider pct={55} /></div>
          <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: 10, color: WF.accentInk, marginTop: 8, fontWeight: 600 }}>
            <span>100 · penumbra</span><span>750 · leitura</span>
          </div>
        </div>
      </div>
      <div style={{ ...padM, marginTop: 16, display: 'flex', flexDirection: 'column', gap: 4 }}>
        <Sub>Ajustes</Sub>
        <Row title="Calibrar sensor" value="último: ontem" icon="dot" />
        <Row title="Limites mín/máx do dimmer" value="10–100%" icon="dot" />
        <Row title="Agendamentos" value="2 ativos" icon="dot" />
        <Row title="Nome e ambiente" value="Mesa · Sala" icon="dot" />
        <Row title="Wi-Fi e conexão" value="CasaWiFi" icon="dot" />
      </div>
      <div style={{ flex: 1 }} />
    </Screen>
  );
}

// ── STATES · offline + automático ──────────────────────────────
function StateOffline() {
  return (
    <Screen label="Estado — Offline">
      <AppBar title="Closet" sub="Quarto"
        left={<IconBtn><Glyph type="back" size={17} c={WF.ink} /></IconBtn>} right={<IconBtn><Glyph type="more" c={WF.ink2} /></IconBtn>} />
      <div style={{ ...padM, display: 'flex', justifyContent: 'center', marginBottom: 14 }}>
        <Chip tone="off"><Dot tone="off" /> OFFLINE · visto há 2h</Chip>
      </div>
      <div style={{ flex: 1, display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', gap: 18, ...padM }}>
        <Dial size={170} pct={0} big="—" unit="sem leitura" cap="dispositivo desconectado" />
        <span style={{ fontSize: 13.5, color: WF.ink2, textAlign: 'center', fontWeight: 500, lineHeight: 1.45 }}>
          não conseguimos falar com este interruptor.<br />verifique a energia e o Wi-Fi.</span>
        <Btn primary>Tentar reconectar</Btn>
        <Btn ghost sm>Ajuda de conexão</Btn>
      </div>
      <TabBar active={0} />
    </Screen>
  );
}

function StateAuto() {
  return (
    <Screen label="Estado — Automático">
      <AppBar title="Mesa de trabalho" sub="Sala de estar"
        left={<IconBtn><Glyph type="back" size={17} c={WF.ink} /></IconBtn>} right={<IconBtn><Glyph type="more" c={WF.ink2} /></IconBtn>} />
      <div style={{ ...padM, display: 'flex', justifyContent: 'center', marginBottom: 8 }}>
        <Chip tone="ok"><Dot tone="ok" /> ONLINE</Chip>
      </div>
      <div style={{ ...padM, marginBottom: 14 }}>
        <Segmented options={['Manual', 'Automático']} active={1} />
      </div>
      <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 6, marginBottom: 14 }}>
        <Dial size={184} pct={100} big="400" unit="lux" cap="✓ no alvo · auto" />
        <span style={{ fontSize: 12.5, color: WF.accentInk, textAlign: 'center', fontWeight: 500, lineHeight: 1.4 }}>
          o sensor mantém 400 lux ajustando<br />a lâmpada conforme a luz natural muda</span>
      </div>
      <div style={{ ...padM, marginBottom: 14 }}>
        {/* dimmer travado no modo auto */}
        <Slider pct={38} label="Dimmer (automático)" muted />
        <div style={{ fontSize: 10, color: WF.ink2, marginTop: 8, textAlign: 'center', fontWeight: 500 }}>controlado pelo sistema · toque em Manual p/ ajustar</div>
      </div>
      <div style={{ ...padM, display: 'flex', gap: 8 }}>
        <Stat value="220" unit="lux" label="luz natural" tone="cool" />
        <Stat value="38" unit="%" label="dimmer" tone="accent" />
        <Stat value="5.1" unit="W" label="consumo" />
      </div>
      <div style={{ flex: 1 }} />
      <TabBar active={0} />
    </Screen>
  );
}

Object.assign(window, { ListA, ListB, DetailA, DetailB, StateOffline, StateAuto });
