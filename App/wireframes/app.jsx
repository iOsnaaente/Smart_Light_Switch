// app.jsx — assembles all wireframe screens into a pan/zoom design canvas.
const { DesignCanvas, DCSection, DCArtboard } = window;
const { Note } = window;
const { LoginA, LoginB, ControlA, ControlB, ControlC,
  ListA, ListB, DetailA, DetailB, StateOffline, StateAuto } = window;

const W = 300;

function App() {
  return (
    <DesignCanvas>
      <DCSection id="login" title="1 · Login" subtitle="Tela de entrada — estática para a demo">
        <DCArtboard id="login-a" label="A · Minimal centrado" width={W} height={628}><LoginA /></DCArtboard>
        <DCArtboard id="login-b" label="B · Hero + social" width={W} height={628}><LoginB /></DCArtboard>
        <Note top={60} right={-214} width={188}>Login é só vitrine na demo — “Entrar como demonstração” pula direto pro controle.</Note>
      </DCSection>

      <DCSection id="controle" title="2 · Controle" subtitle="Tela principal — sensor, dimmer e modo. 3 formas de visualizar o mesmo dado">
        <DCArtboard id="ctrl-a" label="A · Mostrador (dial)" width={W} height={726}><ControlA /></DCArtboard>
        <DCArtboard id="ctrl-b" label="B · Cards empilhados" width={W} height={726}><ControlB /></DCArtboard>
        <DCArtboard id="ctrl-c" label="C · Fader vertical" width={W} height={726}><ControlC /></DCArtboard>
        <Note top={60} right={-222} width={196}>Manual em destaque, Auto como opção. O fader (C) empilha luz natural + lâmpada até o setpoint — bem demonstrativo do produto.</Note>
      </DCSection>

      <DCSection id="dispositivos" title="3 · Meus interruptores" subtitle="Gestão de vários dispositivos">
        <DCArtboard id="list-a" label="A · Lista por ambiente" width={W} height={690}><ListA /></DCArtboard>
        <DCArtboard id="list-b" label="B · Grade de cards" width={W} height={690}><ListB /></DCArtboard>
        <Note top={60} right={-214} width={188}>Toque num item → abre o Controle do interruptor. O menu “···” leva à tela de gerenciamento.</Note>
      </DCSection>

      <DCSection id="gerenciar" title="4 · Gerenciar interruptor" subtitle="Configuração de um dispositivo: setpoint, sensor, Wi-Fi, agendamentos">
        <DCArtboard id="det-a" label="A · Lista de ajustes" width={W} height={668}><DetailA /></DCArtboard>
        <DCArtboard id="det-b" label="B · Setpoint em destaque" width={W} height={668}><DetailB /></DCArtboard>
      </DCSection>

      <DCSection id="estados" title="5 · Estados" subtitle="Como o controle muda em situações diferentes">
        <DCArtboard id="st-auto" label="Automático ativo" width={W} height={726}><StateAuto /></DCArtboard>
        <DCArtboard id="st-off" label="Offline" width={W} height={726}><StateOffline /></DCArtboard>
        <Note top={60} right={-222} width={196}>No modo Auto o dimmer fica travado (o sistema controla). Offline esconde as leituras e oferece reconectar.</Note>
      </DCSection>
    </DesignCanvas>
  );
}

ReactDOM.createRoot(document.getElementById('root')).render(<App />);
