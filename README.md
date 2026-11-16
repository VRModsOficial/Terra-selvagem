# 🏝️ Ilha selvagem *(Beta)*


**Ilha 3D** é um jogo de sobrevivência em primeira pessoa desenvolvido do zero em **C puro**, utilizando **SDL2** e **OpenGL**, sem motores de jogo prontos (engines). 

O projeto ainda está em **fase Beta** e em constante evolução!

---

## ⭐ Apoie o Projeto!
Se você curtiu a ideia de um jogo 3D feito "no osso" em C, achou o projeto interessante ou quer acompanhar as próximas atualizações, **deixe uma estrela (Star ⭐) no topo deste repositório!** Isso ajuda demais a dar visibilidade e motiva a continuar desenvolvendo.

---

## 📖 Sobre o Jogo & Tutorial de Sobrevivência

Após sobreviver à queda de um avião, você acorda isolado em uma ilha deserta e precisa gerenciar seus instintos básicos para continuar vivo.

### 🎬 Início da Jornada (Cutscene)
* Ao iniciar, você verá a cena da queda do avião, a tentativa de pedir socorro e o acidente na montanha direto na janela 3D.
* **Importante:** Fique de olho no **terminal** de onde você executou o jogo — toda a narrativa, inventário e avisos aparecem lá. Pressione `Espaço` para avançar os diálogos.

### 🧭 Como Sobreviver
1. **Gerencie seus Status (Barras na Tela):** Acompanhe as barras coloridas na janela 3D que indicam sua **Vida, Fome, Sede, Energia e Temperatura**.
2. **Hidratação:** Vá até a beira da praia e pressione `E` para beber água (ou crie um Cantil para carregar água com você).
3. **Coleta e Crafting:** Explore a ilha pegando recursos com `E`. Use as teclas de `1` a `5` para fabricar ferramentas essenciais manualmente.
4. **Cace para Comer:** Fabrique uma Lança (`2`) e aproxime-se dos animais pressionando `E` para caçar.
5. **Sobreviva à Noite:** À noite a temperatura cai (a tela ganha um tom azulado) e predadores ficam mais ativos. Crie uma Fogueira (`5`) e posicione-a no chão com `F` para se aquecer, ou use uma Tocha (`3`).
6. **Segredos da Ilha:** Se encontrar um **Ovo Raro**, pressione `O` para chocá-lo!

---

## 🎮 Controles

| Tecla / Ação | Função |
| :--- | :--- |
| **W, A, S, D** | Movimentar o personagem |
| **Mouse** *(clique na janela)* | Olhar ao redor |
| **Esc** | Liberar o cursor do mouse |
| **Shift** | Correr |
| **E** | Coletar item / Beber água na praia / Caçar com a lança |
| **F** | Montar uma fogueira no chão *(se tiver uma criada)* |
| **1** | Craftar **Machado** |
| **2** | Craftar **Lança** |
| **3** | Craftar **Tocha** |
| **4** | Craftar **Cantil** |
| **5** | Craftar **Fogueira** |
| **O** | Chocar o **Ovo Raro** *(se encontrado)* |
| **F5 / F9** | Salvar / Carregar progresso (`ilha_save.txt`) |
| **R** | Recomeçar após morrer |

---

## 🛠️ Dependências para Compilar

Para rodar o jogo, você precisará de um compilador C (`gcc` ou `clang`) e das bibliotecas de desenvolvimento do **SDL2** e **OpenGL**.

* **Ubuntu / Debian:**
  ```bash
  sudo apt install build-essential libsdl2-dev libgl1-mesa-dev
  ```
* **Fedora:**
  ```bash
  sudo dnf install gcc SDL2-devel mesa-libGL-devel
  ```
* **macOS (via Homebrew):**
  ```bash
  brew install sdl2
  ```
  *(Nota: No macOS, caso o `make` reclame de `-lGL` não encontrado, substitua `-lGL` por `-framework OpenGL` no `Makefile`).*
* **Windows (via MSYS2):**
  Instale o pacote `mingw-w64-x86_64-SDL2` e execute o `make` dentro do terminal MSYS2.

---

## 🚀 Como Compilar e Rodar

Clone o repositório, entre na pasta do código-fonte e compile com o `make`:

```bash
git clone https://github.com/VRModsOficial/Terra-selvagem
cd Terra-selvagem
make
./ilha
```

> ⚠️ **Atenção:** Execute o binário `./ilha` **sempre de dentro da pasta `ilha-c/`**, pois os modelos 3D (`models/`) e efeitos sonoros (`sounds/`) são carregados via caminho relativo.

---

## ✨ Recursos Técnicos Implementados

* **Animações Procedurais (Sem Esqueleto):** Balanço das árvores, movimento bamboleante dos animais e predadores, chamas da fogueira tremeluzindo, pulsação do ovo raro e sobrevoo do avião calculados matematicamente em tempo real.
* **Áudio Nativo em SDL2:** 4 efeitos sonoros em `.wav` (pássaros durante o dia, rosnados de predadores à noite, som de água e coleta de itens).
* **Customização de Modelos 3D:** O carregador próprio (`objloader.c`) aceita arquivos `.obj` triangulados (sem materiais externos). Para adicionar cores por vértice, utilize o formato estendido `v x y z r g b` (valores de `0` a `1`) e substitua os arquivos na pasta `models/` (como `palmeira.obj` e `pedra.obj`).

---

## 🚧 Limitações Atuais (Escopo Beta)
* **Interface no Terminal:** Para manter o código enxuto em OpenGL legado sem bibliotecas extras de renderização de fontes, os textos de inventário e história são exibidos no terminal, enquanto a janela 3D exibe as barras de status coloridas.
* **Iluminação Simplificada:** O ciclo dia/noite altera a cor do céu e aplica filtros de temperatura na tela (avermelhado no calor, azulado no frio), sem cálculo de sombras dinâmicas nos objetos.
* **Modelos Primitivos:** Árvores e pedras já utilizam malhas `.obj`, enquanto animais e destroços do avião usam formas geométricas simples (cubos e icosferas) temporariamente.

---

## 🤝 Contribuições e Feedback
Encontrou algum bug na compilação ou tem ideias para melhorar a sobrevivência na ilha? Abra uma **Issue** ou envie um **Pull Request**!
