---

# **Swift Safety IoT - Projeto de Monitoramento Climático**

Este repositório contém o código e as instruções para configurar o sistema **Swift Safety** de monitoramento climático utilizando **Node-RED**, **MQTT**, e **sensores IoT**.

### **Pré-requisitos**

Antes de começar, verifique se você tem os seguintes requisitos instalados no seu ambiente:

* **Node.js** (v14 ou superior)
* **Node-RED** (plataforma para integrar os fluxos IoT)
* **MQTT Broker** (pode ser público ou local, como o Mosquitto)

### **Passo a Passo para Instalação e Execução**

#### 1. **Instalar o Node.js**

Primeiramente, você precisa instalar o Node.js. Se você ainda não tem o Node.js instalado, siga os passos abaixo:

##### **Windows / macOS / Linux**

1. Acesse o site oficial do [Node.js](https://nodejs.org/) e baixe a versão recomendada.
2. Siga as instruções de instalação para o seu sistema operacional.

Verifique se o Node.js foi instalado corretamente executando no terminal:

```bash
node -v
```

#### 2. **Instalar o Node-RED**

Com o Node.js instalado, você pode instalar o **Node-RED** com o seguinte comando:

```bash
npm install -g --unsafe-perm node-red
```

#### 3. **Iniciar o Node-RED**

Após a instalação, inicie o Node-RED com o seguinte comando:

```bash
node-red
```

Isso iniciará o servidor do Node-RED localmente. Você pode acessar a interface do Node-RED em seu navegador, no endereço:

```
http://localhost:1880
```

#### 4. **Clonar o Repositório**

Clone o repositório **Swift Safety IoT** do GitHub para o seu ambiente local:

```bash
git clone https://github.com/giovannarevitoroz/gs-ss-iot.git
```

Entre no diretório do projeto:

```bash
cd gs-ss-iot
```

#### 5. **Instalar Dependências do Node-RED**

O projeto pode ter dependências específicas que precisam ser instaladas para rodar no Node-RED. Para isso, abra o terminal no diretório do projeto clonado e execute:

```bash
npm install
```

#### 6. **Abrir o Fluxo no Node-RED**

1. Acesse a interface do Node-RED no navegador (geralmente em `http://localhost:1880`).

2. No menu principal do Node-RED, clique em "Import" (importar).

3. Selecione o arquivo do fluxo que você deseja importar. Esse arquivo geralmente está em formato `.json` dentro da pasta do projeto clonado.

   * **Exemplo**: Se o fluxo estiver em um arquivo como `flow-swift-safety.json`, selecione esse arquivo.

4. Após a importação, clique em "Deploy" (implantar) para aplicar o fluxo.

#### 7. **Configuração do MQTT**

O sistema depende do protocolo MQTT para enviar e receber dados dos sensores IoT. Para configurar o MQTT:

* Se você está utilizando um broker público, adicione as credenciais e o endereço do broker no fluxo do Node-RED.
* Caso você prefira configurar um broker local, você pode utilizar o [Mosquitto](https://mosquitto.org/), que é um broker MQTT de código aberto. Para instalar o Mosquitto, consulte a [documentação oficial](https://mosquitto.org/download/).

#### 8. **Conectar os Sensores IoT**

Este projeto utiliza sensores conectados ao microcontrolador **ESP32** para coletar dados climáticos. Após configurar o fluxo no Node-RED e o MQTT, você precisará configurar o código do **ESP32** para enviar os dados dos sensores para o broker MQTT.

* O código do ESP32 geralmente será fornecido no repositório ou em outro arquivo dentro do projeto.
* Configure o ESP32 para enviar dados como **temperatura**, **umidade**, **chuva**, **vento**, etc.

#### 9. **Testar o Sistema**

Após ter configurado todos os componentes, execute o sistema e verifique se os dados dos sensores estão sendo recebidos corretamente no Node-RED. Se tudo estiver correto, os alertas e notificações deverão ser gerados conforme as condições climáticas extremas detectadas pelos sensores.

---

### **Documentação do Projeto**

* **GitHub Repository**: [https://github.com/giovannarevitoroz/gs-ss-iot](https://github.com/giovannarevitoroz/gs-ss-iot)
* **Node-RED Documentation**: [https://nodered.org/docs/](https://nodered.org/docs/)
* **MQTT Protocol**: [https://mqtt.org/](https://mqtt.org/)

### **Contribuições**

Sinta-se à vontade para contribuir com melhorias ou sugestões para o projeto. Para isso, basta criar um **pull request** ou abrir uma **issue** no repositório.

---
