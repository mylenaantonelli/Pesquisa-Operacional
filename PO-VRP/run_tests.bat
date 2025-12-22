@echo off
set ITERATIONS=%1
set OUTPUT_FILE=%2

if "%ITERATIONS%"=="" goto missing_args
if "%OUTPUT_FILE%"=="" goto missing_args

echo Iniciando testes em lote...
echo Iteracoes por instancia: %ITERATIONS%
echo Salvando em: %OUTPUT_FILE%
echo ----------------------------------------
echo ---------------------------------------- > %OUTPUT_FILE%

REM O comando abaixo pega todos os arquivos .txt da pasta (exceto este script)
for %%f in (*.txt) do (
    REM Ignora o arquivo de saida se ele for igual ao arquivo de instancia
    if /i "%%f" neq "%OUTPUT_FILE%" (
        echo Processando arquivo: %%f
        echo ---------------------------------------- >> %OUTPUT_FILE%
        echo INSTANCIA: %%f >> %OUTPUT_FILE%
        
        REM Roda o programa com o numero de iteracoes especificado
        vrptw_solver.exe %%f %ITERATIONS% >> %OUTPUT_FILE%
        
        echo Concluido: %%f
    )
)

echo.
echo Todos os testes finalizaram! Verifique o arquivo %OUTPUT_FILE%
goto end

:missing_args
echo ERRO: Parametros faltando.
echo Uso: run_tests.bat [NumeroDeIteracoes] [ArquivoDeSaida.txt]
echo Exemplo: run_tests.bat 5000 relatorio_5000_iters.txt

:end
pause
