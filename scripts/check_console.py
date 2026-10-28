"""Проверка сценариев работы меню через стандартный ввод после сборки."""
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
(ROOT / "tmp").mkdir(exist_ok=True)


def run(name, lines, expected):
    completed = subprocess.run(
        [str(ROOT / "build" / f"{name}.exe")],
        input="\n".join(lines) + "\n", encoding="utf-8",
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=ROOT, timeout=30,
    )
    if completed.returncode:
        raise AssertionError(f"{name} exited with {completed.returncode}: {completed.stderr}")
    for fragment in expected:
        if fragment not in completed.stdout:
            raise AssertionError(f"{name}: missing {fragment!r}\n{completed.stdout}")
    return f"PASS {name} console scenario\n"


report = run("lab1", [
    "1", "4 2 6 1 3 5 7", "5", "6", "7", "2", "8", "( 2 ( 1 # # ) ( 3 # # ) )",
    "9", "10", "bad tree", "5", "1", "99999999999999999999999999999", "5",
    "11", "20", "7", "10", "13",
    "12", "1", "1 2 3", "2", "3 4", "3", "7", "8", "9", "10", "11", "12",
    "13", "( 2 ( 1 # # ) ( 3 # # ) )", "4", "9", "5", "9", "6", "1", "0", "0",
], ["Узлов: 7; высота: 3", "reduce(сумма): 28", "Входит", "Ошибка:",
    "{ 1 2 3 4 } (4)", "{ 3 } (1)", "{ 1 2 } (2)", "hello world"])

report += run("lab2", [
    "10", "examples/vector.txt", "1", "4", "3", "1", "3", "11", "12", "11",
    "5", "2", "2", "4", "0", "9", "tmp/roundtrip_vector.txt",
    "10", "tmp/roundtrip_vector.txt", "1", "4",
    "6", "12", "2", "1", "0", "-2.5",
    "7", "0", "8", "12", "2", "1", "11", "2",
    "3", "12", "10", "tmp/nonexistent_file.txt", "1",
    "11", "20", "5", "42", "2", "4", "0",
], ["Размерность 12, ненулевых 3", "Значение: 0", "Значение: 7", "Binary search",
    "Размерность 12, ненулевых 2", "Скалярное произведение: 14", "Ошибка:", "Создано ненулевых: 5"])

saved = (ROOT / "tmp" / "roundtrip_vector.txt").read_text(encoding="utf-8")
if saved != "SPARSE_VECTOR 1\n12 2\n0 2.5\n11 7\n":
    raise AssertionError(f"Unexpected saved vector: {saved!r}")
report += "PASS actual file save/load through menu\n"

# Конец перенаправленного ввода должен завершать программу.
for name in ["lab1", "lab2"]:
    completed = subprocess.run([str(ROOT / "build" / f"{name}.exe")], input="", encoding="utf-8",
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=5)
    if completed.returncode:
        raise AssertionError(f"{name}: EOF exit failed")
report += "PASS EOF handling in both consoles\n"
(ROOT / "results" / "console_checks.txt").write_text(report, encoding="utf-8")
print(report)
