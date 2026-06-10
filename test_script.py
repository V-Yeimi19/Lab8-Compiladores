import os
import sys
import difflib
from rich.console import Console
from rich.panel import Panel
from rich.text import Text
from rich.table import Table

console = Console()

def format_diff(expected, actual, filename):
    """
    Formatea la diferencia usando colores de Rich.
    """
    diff = difflib.unified_diff(
        expected.splitlines(keepends=True),
        actual.splitlines(keepends=True),
        fromfile=f'TEST {filename}',
        tofile=f'GENERADO {filename}'
    )
    
    text = Text()
    for line in diff:
        if line.startswith("---") or line.startswith("+++"):
            text.append(line, style="bold cyan")
        elif line.startswith("@@"):
            text.append(line, style="cyan")
        elif line.startswith("+"):
            text.append(line, style="bold green")
        elif line.startswith("-"):
            text.append(line, style="bold red")
        else:
            text.append(line, style="dim white")
            
    return Panel(text, title=f"Diferencia en [bold cyan]{filename}[/]", border_style="red")

def compare_outputs(test_dir, out_dir):
    if not os.path.exists(test_dir):
        console.print(f"[bold red]Error:[/bold red] El directorio de test '{test_dir}' no existe.")
        return False
        
    if not os.path.exists(out_dir):
        console.print(f"[bold red]Error:[/bold red] El directorio de outputs '{out_dir}' no existe.")
        return False

    archivos_test = [f for f in sorted(os.listdir(test_dir)) if f.endswith(".s")]
    
    if not archivos_test:
        console.print(f"[bold yellow]Advertencia:[/bold yellow] No se encontraron archivos .s en {test_dir}")
        return False

    console.print(Panel(f"[bold blue]Iniciando Testing de Código Ensamblador...[/bold blue]\nDirectorio Tests: [cyan]{test_dir}[/cyan]\nDirectorio Generados: [cyan]{out_dir}[/cyan]", border_style="blue"))

    all_passed = True
    results = []

    for filename in archivos_test:
        test_path = os.path.join(test_dir, filename)
        out_path = os.path.join(out_dir, filename)
        
        if not os.path.exists(out_path):
            console.print(f"[bold red]✗[/bold red] {filename}: Falta el archivo generado")
            results.append((filename, False, "Archivo faltante"))
            all_passed = False
            continue
            
        file_passed = True
        errors = []
        
        try:
            with open(test_path, 'r', encoding='utf-8') as f:
                expected = f.read()
            with open(out_path, 'r', encoding='utf-8') as f:
                actual = f.read()
            
            if expected != actual:
                file_passed = False
                all_passed = False
                console.print(f"\n[bold red]Falló {filename}[/bold red]")
                console.print(format_diff(expected, actual, filename))
        except Exception as e:
            file_passed = False
            all_passed = False
            errors.append(f"Error al leer/comparar: {str(e)}")
        
        if file_passed:
            results.append((filename, True, "OK"))
        else:
            results.append((filename, False, ", ".join(errors) if errors else "Diferencias en código ensamblador"))

    # Mostrar tabla resumen
    console.print("\n")
    table = Table(title="Resumen de Resultados", border_style="magenta", show_lines=True)
    table.add_column("Archivo", justify="left", style="cyan", no_wrap=True)
    table.add_column("Estado", justify="center")
    table.add_column("Detalle", justify="left")

    for filename, passed, detail in results:
        status = "[bold green]✓ PASSED[/bold green]" if passed else "[bold red]✗ FAILED[/bold red]"
        detail_style = "dim" if passed else "red"
        table.add_row(filename, status, f"[{detail_style}]{detail}[/{detail_style}]")

    console.print(table)
    
    return all_passed

if __name__ == "__main__":
    test_dir = sys.argv[1] if len(sys.argv) > 1 else "outputs_test"
    out_dir = sys.argv[2] if len(sys.argv) > 2 else "outputs"
    
    success = compare_outputs(test_dir, out_dir)
    
    console.print("\n")
    if success:
        console.print(Panel("[bold green]✨ TODOS LOS TESTS PASARON CORRECTAMENTE ✨[/bold green]", border_style="green"))
        sys.exit(0)
    else:
        console.print(Panel("[bold red]❌ ALGUNOS TESTS FALLARON. REVISA LOS DETALLES ARRIBA ❌[/bold red]", border_style="red"))
        sys.exit(1)
