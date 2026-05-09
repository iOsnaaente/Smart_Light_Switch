# TriacController

Controle de potência AC usando TRIAC sincronizado por zero-cross.

## O que faz

O `TriacController` controla a potência entregue a uma carga AC utilizando acionamento por defasagem de fase. A aplicação apenas define um setpoint entre `0.0` e `1.0`, enquanto o módulo gerencia internamente:

* Sincronismo com a rede;
* Detecção de zero-cross;
* Temporização;
* Cálculo do ângulo de disparo;
* Acionamento do gate do TRIAC;
* Estados operacionais.

### O que é acionamento por defasagem de fase

O controle por defasagem de fase funciona atrasando o instante em que o TRIAC é acionado dentro de cada semiciclo de 60Hz da rede AC. A cada passagem por zero da senoide, o controller:

1. Detecta o início do semiciclo;
2. Aguarda um tempo calculado;
3. Dispara o TRIAC;
4. O TRIAC permanece conduzindo até o próximo zero-cross.

Quanto menor o atraso, maior o tempo de condução da carga e maior a potência entregue. Quanto maior o atraso, menor a potência.

Exemplo simplificado:

* disparo imediato -> potência máxima;
* disparo no meio do semiciclo -> potência média;
* disparo próximo do final -> potência baixa.

O setpoint controla diretamente esse atraso. A potência não é linear com a defasagem de fase!

### Por que foi desenhado assim

O módulo foi separado em ISR + task para manter o controle temporal preciso sem bloquear interrupções. A ISR executa apenas operações rápidas:

* Captura timestamp do ZeroCross;
* Incrementa contador;
* Acorda a task.

Toda a lógica de controle fica na task FreeRTOS. Isso permite:

* Menor latência;
* Temporização em microssegundos;
* Debounce interno;
* Operação mais estável;
* Sincronismo contínuo com a rede.

O módulo também consegue continuar operando mesmo sem zero-cross válido, usando o último semiciclo conhecido como referência de fallback.

## Como funciona

1. O detector de zero-cross gera interrupções;
2. A ISR salva o timestamp do evento;
3. A task principal mede o semiciclo da rede;
4. O atraso de disparo é calculado pelo setpoint;
5. O gate do TRIAC é acionado;
6. O status interno é atualizado.

## Como usar

### Inicialização

```cpp
#define GPIO_ZERO_CROSS GPIO_NUM_4
#define GPIO_TRIAC      GPIO_NUM_5

TriacController triac;

triac.init(
    GPIO_ZERO_CROSS,
    GPIO_TRIAC
);
triac.start();
```

### Controle

```cpp
    triac.set_setpoint(0.75f);
```

* `0.0f` -> desligado;
* `1.0f` -> totalmente ligado.

O Setpoint pode ser originado de:

1. Sensor LDR (luminosidade)
2. Tela Touch do interruptor
3. Aplicativo

Por este motivo que a interface de controle apenas expoe o método de Setpoint;

### Leitura de estado

```cpp
float sp = triac.get_setpoint();
LampStatus_t status = triac.get_status();
```

### Encerramento

```cpp
triac.stop();
```

## Estados

| Estado                | Descrição              |
| --------------------- | ---------------------- |
| `LAMP_STATUS_OK`      | Operação normal        |
| `LAMP_FULLY_ON`       | Potência máxima        |
| `LAMP_FULLY_OFF`      | Potência zero          |
| `LAMP_STATUS_OFFLINE` | Sem sincronismo válido |
| `LAMP_STATUS_ERROR`   | Erro interno           |

## Parâmetros importantes

| Parâmetro               | Função                            |
| ----------------------- | --------------------------------- |
| `zero_cross_gpio`       | Entrada do detector de zero-cross |
| `triac_gate_gpio`       | Saída do gate do TRIAC            |
| `debounce_us`           | Ignora ruído no zero-cross        |
| `triac_gate_pulse_us`   | Largura do pulso de gate          |
| `nominal_half_cycle_us` | Semiciclo padrão                  |
| `min_half_cycle_us`     | Limite mínimo do semiciclo        |
| `max_half_cycle_us`     | Limite máximo do semiciclo        |
| `zc_timeout_us`         | Timeout sem zero-cross            |

## Dependências

* ESP-IDF;
* FreeRTOS;
* GPIO Driver;
* esp_timer;
* GPIO ISR Service.

## Restrições

* Requer detector de zero-cross externo;
* Indicado para cargas AC;
* Depende da estabilidade temporal do sistema;
* Não é thread-safe fora das APIs protegidas.
