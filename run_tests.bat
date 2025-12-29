@echo off

REM Leitura dos parametros
set ITERATIONS=%1
set OUTPUT_FILE=%2
set INST_DIR=instancias
set EXEC=bin\vrptw_solver.exe

REM Verificacao dos parametros
if "%ITERATIONS%"=="" (
    echo ERRO: Numero de iteracoes nao informado.
    echo Uso: run_tests.bat [Iteracoes] [ArquivoSaida.txt]
    pause
    exit /b
)

if "%OUTPUT_FILE%"=="" (
    echo ERRO: Arquivo de saida nao informado.
    echo Uso: run_tests.bat [Iteracoes] [ArquivoSaida.txt]
    pause
    exit /b
)

REM Verifica executavel
if not exist "%EXEC%" (
    echo ERRO: Executavel nao encontrado em %EXEC%
    pause
    exit /b
)

echo Iniciando testes em lote...
echo ---------------------------------------- > "%OUTPUT_FILE%"

REM Loop pelas instancias
for %%f in (%INST_DIR%\*.txt) do (
    echo Processando arquivo: %%f
    echo ---------------------------------------- >> "%OUTPUT_FILE%"
    echo INSTANCIA: %%f >> "%OUTPUT_FILE%"
    
    "%EXEC%" "%%f" %ITERATIONS% >> "%OUTPUT_FILE%"
    
    echo Concluido: %%f
)

echo.
echo Todos os testes finalizaram!
echo Resultados em: %OUTPUT_FILE%
pause

