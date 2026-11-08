# 🚀 Notepad++ Linux MCP Server (`npp-mcp`)

Servidor oficial de ferramentas **Model Context Protocol (MCP)** para integração do **Notepad++ v8.9.8.1 nativo no Linux** com agentes de inteligência artificial (Google Antigravity, Claude Code, Cline, Cursor, etc.).

---

## 🛠️ Ferramentas Expostas aos Agentes de IA

| Ferramenta MCP | Descrição |
| :--- | :--- |
| `npp_ping` | Verifica a integridade da instância, PID, abas ativas e versão do editor. |
| `npp_list_tabs` | Lista todas as abas abertas, títulos, caminhos de arquivo, quantidade de linhas e estado `isModified`. |
| `npp_open_file(file_path, line)` | Abre qualquer arquivo em nova aba (ou foca se já aberto) e salta para a linha especificada. |
| `npp_get_active_buffer` | Extrai o buffer de texto completo, linha e coluna atual, e seleção de texto da aba ativa. |
| `npp_set_active_buffer(text)` | Substitui o texto completo do buffer ativo no Scintilla. |
| `npp_insert_text(text)` | Insere texto na posição atual do cursor. |
| `npp_apply_diff(start_pos, length, replacement)` | Aplica um patch/diff cirúrgico no buffer ativo sem sobrescrever o arquivo inteiro. |
| `npp_new_tab(initial_text)` | Cria uma nova aba em branco, opcionalmente com código inicial. |
| `npp_save_active_file` | Salva o documento ativo no disco. |
| `npp_switch_tab(index)` | Alterna para a aba informada pelo índice (0-indexed). |
| `npp_close_tab(index)` | Fecha a aba informada (ou a ativa se omitido). |
| `npp_list_themes` | Lista todos os 22 temas XML oficiais (Monokai, Obsidian, Solarized, Zenburn, etc.). |
| `npp_set_theme(theme_name)` | Aplica instantaneamente um tema visual ao vivo. |
| `npp_toggle_function_list(visible)` | Exibe ou oculta o painel lateral Function List de navegação de classes e funções. |
| `npp_show_notification(message, timeout_ms)` | Exibe uma notificação visual na barra de status do Notepad++ para o usuário. |
| `npp_save_session(path)` | Salva a sessão ativa (arquivos abertos, cursores e aba ativa) em `session.xml`. |
| `npp_load_session(path)` | Restaura as abas e cursores a partir de um arquivo `session.xml`. |

---

## ⚙️ Configuração no Antigravity / Claude Code / Cline

Adicione o servidor ao seu arquivo de configuração MCP (`~/.config/Claude/claude_desktop_config.json` ou similar):

```json
{
  "mcpServers": {
    "notepadpp": {
      "command": "python3",
      "args": [
        "/home/namerico/Documentos/Projetos Antigravity/notepadpp-agent-linux/mcp-server/npp_mcp.py"
      ]
    }
  }
}
```

---

## 🧪 Testando o Servidor Localmente

Você pode iniciar o servidor MCP e verificar suas ferramentas via CLI ou MCP Inspector:

```bash
# Execução direta via Python 3:
python3 /home/namerico/Documentos/Projetos Antigravity/notepadpp-agent-linux/mcp-server/npp_mcp.py
```
