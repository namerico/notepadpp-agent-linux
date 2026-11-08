#!/usr/bin/env python3
"""
Notepad++ Agent Linux - Native MCP Server (npp-mcp)
Exposes Notepad++ v8.9.8.1 native Linux editor controls to AI agents via Model Context Protocol (MCP).
"""

import sys
import os
import socket
import json
import time
from typing import Dict, Any, List, Optional
from mcp.server.mcpserver import MCPServer

SOCKET_PATH = "/tmp/npp_antigravity.sock"
BINARY_PATH = "/home/namerico/Documentos/Projetos Antigravity/notepadpp-agent-linux/src/npp-linux/build/notepadpp-agent"

def send_rpc(method: str, params: Optional[Dict[str, Any]] = None, timeout: float = 3.0) -> Dict[str, Any]:
    if params is None:
        params = {}

    if not os.path.exists(SOCKET_PATH):
        # Auto-launch if socket is not found
        import subprocess
        env = os.environ.copy()
        if "DISPLAY" not in env:
            env["DISPLAY"] = ":0"
        subprocess.Popen([BINARY_PATH], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, start_new_session=True)
        for _ in range(25):
            time.sleep(0.1)
            if os.path.exists(SOCKET_PATH):
                break

    if not os.path.exists(SOCKET_PATH):
        return {"error": f"Notepad++ socket not available at {SOCKET_PATH}"}

    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(timeout)
    try:
        s.connect(SOCKET_PATH)
        req = {
            "jsonrpc": "2.0",
            "id": int(time.time() * 1000) % 100000,
            "method": method,
            "params": params
        }
        s.sendall(json.dumps(req).encode('utf-8') + b"\n")

        buffer = b""
        while True:
            chunk = s.recv(4096)
            if not chunk:
                break
            buffer += chunk
            if b"\n" in buffer:
                break

        res = json.loads(buffer.decode('utf-8').strip())
        return res
    except Exception as e:
        return {"error": f"Socket communication error: {str(e)}"}
    finally:
        s.close()

# Inicializa o servidor MCP
server = MCPServer("npp-mcp", version="1.0.0")

@server.tool()
def npp_ping() -> str:
    """Verifica se o Notepad++ está ativo e retorna status, PID e contagem de abas."""
    res = send_rpc("ping")
    return json.dumps(res, indent=2, ensure_ascii=False)

@server.tool()
def npp_list_tabs() -> str:
    """Lista todas as abas abertas no Notepad++, seus caminhos de arquivo, linhas e estado de modificação."""
    res = send_rpc("listTabs")
    return json.dumps(res, indent=2, ensure_ascii=False)

@server.tool()
def npp_open_file(file_path: str, line: int = 0) -> str:
    """Abre um arquivo no Notepad++ em nova aba (ou foca se já aberto) e posiciona na linha indicada."""
    res = send_rpc("openFile", {"path": os.path.abspath(file_path), "line": line})
    return json.dumps(res, indent=2, ensure_ascii=False)

@server.tool()
def npp_get_active_buffer() -> str:
    """Extrai o texto completo, posição do cursor, seleções e metadados da aba ativa."""
    res = send_rpc("getActiveBuffer")
    return json.dumps(res, indent=2, ensure_ascii=False)

@server.tool()
def npp_set_active_buffer(text: str) -> str:
    """Substitui o conteúdo integral da aba ativa pelo novo texto fornecido."""
    res = send_rpc("setActiveBuffer", {"text": text})
    return json.dumps(res, indent=2, ensure_ascii=False)

@server.tool()
def npp_insert_text(text: str) -> str:
    """Insere o texto fornecido na posição atual do cursor no buffer ativo."""
    res = send_rpc("insertText", {"text": text})
    return json.dumps(res, indent=2, ensure_ascii=False)

@server.tool()
def npp_apply_diff(start_pos: int, length: int, replacement: str) -> str:
    """Aplica uma modificação cirúrgica no buffer ativo do Scintilla por intervalo de bytes (start_pos e length)."""
    res = send_rpc("applyDiff", {"startPos": start_pos, "length": length, "replacement": replacement})
    return json.dumps(res, indent=2, ensure_ascii=False)

@server.tool()
def npp_new_tab(initial_text: str = "") -> str:
    """Cria uma nova aba vazia no Notepad++, opcionalmente preenchendo com texto inicial."""
    res = send_rpc("newTab", {"text": initial_text})
    return json.dumps(res, indent=2, ensure_ascii=False)

@server.tool()
def npp_save_active_file() -> str:
    """Salva o documento ativo no disco."""
    res = send_rpc("saveActiveFile")
    return json.dumps(res, indent=2, ensure_ascii=False)

@server.tool()
def npp_switch_tab(index: int) -> str:
    """Alterna o foco para a aba especificada pelo índice (0-based)."""
    res = send_rpc("switchTab", {"index": index})
    return json.dumps(res, indent=2, ensure_ascii=False)

@server.tool()
def npp_close_tab(index: int = -1) -> str:
    """Fecha a aba indicada (ou a aba ativa caso omitido)."""
    params = {}
    if index >= 0:
        params["index"] = index
    res = send_rpc("closeTab", params)
    return json.dumps(res, indent=2, ensure_ascii=False)

@server.tool()
def npp_list_themes() -> str:
    """Lista todos os 22 temas XML oficiais disponíveis (Monokai, Obsidian, Solarized, Zenburn, etc.)."""
    res = send_rpc("listThemes")
    return json.dumps(res, indent=2, ensure_ascii=False)

@server.tool()
def npp_set_theme(theme_name: str) -> str:
    """Aplica um tema visual ao vivo no Notepad++ (ex: Monokai, Obsidian, Zenburn, Padrão)."""
    res = send_rpc("setTheme", {"theme": theme_name})
    return json.dumps(res, indent=2, ensure_ascii=False)

@server.tool()
def npp_toggle_function_list(visible: bool) -> str:
    """Exibe ou oculta o painel lateral Function List (navegador de classes e funções)."""
    res = send_rpc("toggleFunctionList", {"visible": visible})
    return json.dumps(res, indent=2, ensure_ascii=False)

@server.tool()
def npp_show_notification(message: str, timeout_ms: int = 4000) -> str:
    """Exibe uma mensagem informativa na barra de status do Notepad++ para o usuário."""
    res = send_rpc("showNotification", {"message": message, "timeout": timeout_ms})
    return json.dumps(res, indent=2, ensure_ascii=False)

@server.tool()
def npp_save_session(path: str = "") -> str:
    """Salva a sessão atual (arquivos abertos, linha e aba ativa) em session.xml."""
    res = send_rpc("saveSession", {"path": path})
    return json.dumps(res, indent=2, ensure_ascii=False)

@server.tool()
def npp_load_session(path: str = "") -> str:
    """Restaura as abas e cursores a partir de um arquivo session.xml."""
    res = send_rpc("loadSession", {"path": path})
    return json.dumps(res, indent=2, ensure_ascii=False)

@server.tool()
def npp_set_padding(left: int = 16, right: int = 16, top: int = 6, bottom: int = 4) -> str:
    """Ajusta o respiro/padding das margens do código (laterais e verticais no topo/base de cada linha)."""
    res = send_rpc("setPadding", {"left": left, "right": right, "top": top, "bottom": bottom})
    return json.dumps(res, indent=2, ensure_ascii=False)

@server.tool()
def npp_get_padding() -> str:
    """Consulta os valores atuais de padding e margens do editor."""
    res = send_rpc("getPadding")
    return json.dumps(res, indent=2, ensure_ascii=False)

if __name__ == "__main__":
    # Inicia o servidor MCP via stdio
    import asyncio
    asyncio.run(server.run_stdio_async())
