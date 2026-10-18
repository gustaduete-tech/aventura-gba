# Aventura GBA — protótipo RPG de exploração

Protótipo em C para Game Boy Advance, inspirado em aventuras de exploração top-down da era 16-bit. A tela de título usa a arte fornecida, reduzida para 240×160 e convertida para RGB555. O mundo de jogo é desenhado em pixel art diretamente no framebuffer Mode 3.

## Recursos já implementados
- Mundo 64×64 tiles, com câmera que acompanha o jogador.
- Exploração top-down, colisão com água, árvores e pedras.
- Movimento com D-pad; espada com A.
- Cristais colecionáveis, HUD de vida e progresso, mensagens e condição de vitória.
- Árvores e pedras podem ser cortadas/quebradas com a espada.
- Arte da tela inicial incluída em `src/title_data.h`.

## Controles
- **START**: sair da tela de título e começar.
- **Setas**: mover o personagem.
- **A**: atacar/interagir com a espada.

## Compilar
Instale o toolchain **devkitPro/devkitARM** com suporte a GBA. Em um terminal, dentro desta pasta:

```sh
make
```

O resultado será `aventura.gba`, que pode ser aberto em um emulador de GBA (por exemplo, mGBA) ou transferido para um flashcart compatível. Se o seu ambiente não fornecer `gba.specs`, use o template oficial de projeto GBA do devkitPro e adapte as regras de linkedição.

## Próximas melhorias recomendadas
Este é um protótipo jogável, não um RPG completo. Para evoluir: trocar o desenho por tilesets/sprites dedicados, adicionar NPCs e diálogos, inventário, cavernas, salvamento, música e mapas feitos à mão. O projeto usa desenho em Mode 3 para manter o código inicial simples; um jogo maior deve migrar para tiles/sprites em modo de fundo para aproveitar melhor a memória e o desempenho do GBA.
