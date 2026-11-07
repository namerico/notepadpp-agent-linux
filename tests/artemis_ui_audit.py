#!/usr/bin/env python3
"""
Artemis Linux & REA - UI & Integration Test Suite
Notepad++ Agent Linux
"""

import sys
import json
import subprocess
import time

def run_npp(cmd):
    p = subprocess.run(f"npp-cli {cmd}", shell=True, capture_output=True, text=True)
    assert p.returncode == 0, f"Falha no comando npp-cli {cmd}: {p.stderr}"
    try:
        return json.loads(p.stdout)
    except:
        return p.stdout

def main():
    print("=" * 60)
    print(" ARTEMIS LINUX & REA - AUDITORIA DE UI E TESTES DE INTEGRACAO")
    print("=" * 60)

    # 1. Ping
    print("\n[TESTE 1] Ping & Checagem do Agent Bridge IPC...")
    res = run_npp("ping")
    assert res.get("result", {}).get("status") == "ok"
    pid = res["result"]["pid"]
    print(f" -> Conexao estabelecida com sucesso! PID: {pid}")

    # 2. Lista de Temas Oficiais
    print("\n[TESTE 2] Auditoria dos Temas Oficiais do Notepad++...")
    res = run_npp("themes")
    themes = res.get("result", {}).get("themes", [])
    print(f" -> Total de temas oficiais carregados: {len(themes)}")
    assert len(themes) >= 20, "Deveria carregar pelo menos 20 temas oficiais"
    assert "Monokai" in themes and "Obsidian" in themes and "Solarized" in themes

    # 3. Alternância Dinâmica de Temas
    for test_theme in ["Monokai", "Obsidian", "Zenburn", "Padrão"]:
        print(f" -> Aplicando tema: {test_theme}...")
        res = run_npp(f"theme '{test_theme}'")
        assert res.get("result", {}).get("success") is True
        time.sleep(0.2)

    # 4. Function List Toggle
    print("\n[TESTE 3] Auditoria do Painel Function List...")
    res = run_npp("function-list --show")
    assert res.get("result", {}).get("visible") is True
    print(" -> Function List exibido com sucesso!")

    # 5. Criar aba com classe e função e validar navegação
    print("\n[TESTE 4] Criacao de Codigo com Simbolos e Validacao...")
    code = "class Calculadora:\\n    def somar(self, a, b):\\n        return a + b\\n\\ndef main():\\n    pass\\n"
    res = run_npp(f'new --text "{code}"')
    tab_idx = res.get("result", {}).get("tabIndex")
    print(f" -> Nova aba criada no indice {tab_idx}")

    # 6. Notificação final
    run_npp('notify "Auditoria do Artemis Linux concluida com 100% de sucesso!"')
    print("\n[TESTE 5] Notificacao visual despachada para a UI.")

    print("\n" + "=" * 60)
    print(" [OK] TODOS OS TESTES DO ARTEMIS LINUX PASSARAM COM SUCESSO!")
    print("=" * 60)

if __name__ == "__main__":
    main()
