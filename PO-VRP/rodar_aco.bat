@echo off
echo ==========================================
echo      RODANDO ACO (COLONIA DE FORMIGAS)
echo ==========================================
echo Iniciando testes em lote...
REM Cria um arquivo de log NOVO e SEPARADO
echo ---------------------------------------- > RESULTADOS_ACO.LOG

REM Procura apenas arquivos de instancia (c*.txt, r*.txt, rc*.txt)
for %%f in (c*.txt r*.txt rc*.txt) do (
    echo Processando arquivo: %%f
    echo ---------------------------------------- >> RESULTADOS_ACO.LOG
    echo INSTANCIA: %%f >> RESULTADOS_ACO.LOG
    
    REM Executa o ACO com 100 iterações (ajuste se quiser mais)
    aco_solver.exe %%f 100 >> RESULTADOS_ACO.LOG
    
    echo Concluido: %%f
)

echo.
echo Testes ACO finalizados! Verifique o arquivo RESULTADOS_ACO.LOG
pause