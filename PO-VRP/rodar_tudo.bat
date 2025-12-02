@echo off
echo Iniciando testes em lote...
echo ---------------------------------------- > relatorio_final.txt

REM O comando abaixo pega todos os arquivos .txt da pasta
for %%f in (*.txt) do (
    echo Processando arquivo: %%f
    echo ---------------------------------------- >> relatorio_final.txt
    echo INSTANCIA: %%f >> relatorio_final.txt
    
    REM Aqui ele roda seu programa (1000 iteracoes) e joga o resultado no arquivo
    vrptw_solver.exe %%f 1000 >> relatorio_final.txt
    
    echo Concluido: %%f
)

echo.
echo Todos os testes finalizaram! Verifique o arquivo relatorio_final.txt
pause