# LDRSensor

Driver para sensor LDR utilizando ADC do ESP32 com leitura periódica e filtro EMA. 

## O que faz

O `LDRSensor` realiza a leitura contínua de um sensor LDR utilizando o ADC do ESP32.

O módulo abstrai a lógica interna de aquisição e expõe apenas uma interface simples para leitura dos valores processados.

Principais recursos internos gerenciados automaticamente:

* Configuração do ADC;
* Leitura periódica do sensor;
* Filtragem EMA;
* Conversão para tensão;
* Normalização dos valores;
* Gerenciamento da task de aquisição.

## Conceito principal

O LDR (Light Dependent Resistor) altera sua resistência conforme a luminosidade ambiente.

O circuito converte essa variação em uma tensão analógica que é medida pelo ADC do ESP32.

Internamente o módulo:

1. Realiza leituras periódicas do ADC;
2. Aplica filtro EMA nos valores;
3. Atualiza o valor filtrado;
4. Disponibiliza o resultado para a aplicação.

O parâmetro `alpha` controla a resposta do filtro:

* `alpha` baixo -> leitura mais estável;
* `alpha` alto -> leitura mais rápida.

O módulo pode retornar:

* Valor bruto do ADC;
* Valor normalizado entre `0.0` e `1.0`;
* Valor em tensão.

Observações importantes:

* O ADC do ESP32 não é perfeitamente linear;
* O valor normalizado não representa lux reais;
* O filtro adiciona atraso na resposta;
* A leitura depende da alimentação e do hardware analógico.

## Por que foi desenhado assim

O módulo utiliza uma task dedicada para desacoplar a aquisição do sensor da aplicação principal.

Isso permite:

* Taxa de aquisição constante;
* Menor carga na aplicação;
* Filtragem contínua;
* Reutilização em múltiplos sistemas.

O filtro EMA foi escolhido por:

* Baixo custo computacional;
* Pouco uso de memória;
* Boa suavização;
* Resposta contínua.

O acesso às variáveis utiliza seção crítica para garantir thread safety entre task e aplicação.

## Como funciona

1. O ADC é configurado;
2. Uma task periódica é criada;
3. O ADC é lido periodicamente;
4. O filtro EMA atualiza o valor filtrado;
5. A aplicação consulta os valores processados.

## Como usar

### Inicialização

```cpp id="tk9f4n"
LDRSensor ldr;

ldr.init(
    ADC_UNIT_1,
    ADC_CHANNEL_0,
    100,
    ADC_ATTEN_DB_12,
    ADC_BITWIDTH_12,
    0.2f
);

ldr.start();
```

Parâmetros principais:

* Unidade ADC;
* Canal ADC;
* Período de leitura;
* Atenuação;
* Resolução;
* Fator EMA.

### Controle

O módulo não possui métodos de controle ativos.

A aplicação apenas configura os parâmetros de aquisição e leitura.

Os valores podem ser utilizados por:

* Controle de luminosidade;
* Interfaces automáticas;
* Controle PID;
* Sensoriamento ambiental;
* Ajuste automático de brilho.

A API expõe apenas métodos de leitura para manter o sensor desacoplado da lógica da aplicação.

### Leitura de estado

```cpp id="f7m2qb"
float normalized;
float voltage;
uint32_t raw;

ldr.read_normalized(&normalized);
ldr.read_voltage(&voltage);
ldr.read_raw(&raw);
```

Métodos públicos principais:

* `read_raw()`
* `read_voltage()`
* `read_normalized()`

## Encerramento

```cpp id="n8z5kc"
ldr.stop();
ldr.deinit();
```

O módulo:

* Encerra a task de leitura;
* Libera os recursos do ADC;
* Remove os handles internos.

## Estados

| Estado                  | Descrição          |
| ----------------------- | ------------------ |
| `ESP_OK`                | Operação normal    |
| `ESP_ERR_INVALID_STATE` | Estado inválido    |
| `ESP_ERR_INVALID_ARG`   | Argumento inválido |

## Parâmetros importantes

| Parâmetro        | Função                   |
| ---------------- | ------------------------ |
| `read_period_ms` | Período de aquisição     |
| `alpha`          | Suavização do filtro EMA |
| `atten`          | Faixa de tensão do ADC   |
| `bitwidth`       | Resolução do ADC         |
| `filtered`       | Valor filtrado atual     |

## Dependências

* ESP-IDF;
* FreeRTOS;
* ADC Oneshot Driver;
* esp_log;
* GPIO Driver.

## Restrições

* Depende do ADC do ESP32;
* Sensível a ruído elétrico;
* O ADC do ESP32 possui não-linearidades;
* O valor normalizado não representa lux absolutos;
* Requer hardware analógico adequado.
