#!/usr/bin/env python3
"""
Скрипт для построения поискового индекса через C++ модули
"""

import argparse
import subprocess
import sys
import os
from pathlib import Path

def build_cpp_modules():
    """Собирает C++ модули"""
    print("Building C++ modules...")
    
    build_script = Path(__file__).parent.parent / "scripts" / "build_cpp.sh"
    
    if not build_script.exists():
        print(f"Error: Build script not found: {build_script}")
        return False
    
    try:
        result = subprocess.run(
            ["bash", str(build_script)],
            cwd=Path(__file__).parent.parent,
            capture_output=True,
            text=True
        )
        
        if result.returncode != 0:
            print(f"Build failed:\n{result.stderr}")
            return False
        
        print(result.stdout)
        return True
        
    except Exception as e:
        print(f"Error building C++ modules: {e}")
        return False

def run_tokenizer(input_file, output_file, stopwords_path=None):
    """Запускает токенизатор"""
    print(f"Tokenizing {input_file}...")
    
    tokenizer_bin = Path(__file__).parent.parent / "bin" / "tokenizer"
    
    if not tokenizer_bin.exists():
        print(f"Error: Tokenizer binary not found: {tokenizer_bin}")
        return False
    
    cmd = [str(tokenizer_bin), input_file]
    
    if stopwords_path:
        cmd.extend(["--stopwords", stopwords_path])
    
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True
        )
        
        if result.returncode != 0:
            print(f"Tokenizer failed:\n{result.stderr}")
            return False
        
        # Сохраняем результат
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(result.stdout)
        
        print(f"Tokens saved to {output_file}")
        return True
        
    except Exception as e:
        print(f"Error running tokenizer: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(description='Build search index from documents')
    parser.add_argument('--input', type=str, required=True,
                       help='Input file or directory with documents')
    parser.add_argument('--output', type=str, default='data/indexes',
                       help='Output directory for indexes')
    parser.add_argument('--stopwords', type=str, 
                       help='Path to stopwords file')
    parser.add_argument('--rebuild', action='store_true',
                       help='Rebuild C++ modules')
    parser.add_argument('--benchmark', action='store_true',
                       help='Run benchmarks')
    
    args = parser.parse_args()
    
    # Проверяем существование входного файла/директории
    input_path = Path(args.input)
    if not input_path.exists():
        print(f"Error: Input path does not exist: {args.input}")
        return 1
    
    # Создаем выходную директорию
    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Собираем C++ модули, если нужно
    if args.rebuild:
        if not build_cpp_modules():
            return 1
    
    # Запускаем токенизацию
    if input_path.is_file():
        # Один файл
        output_file = output_dir / f"{input_path.stem}_tokens.txt"
        if not run_tokenizer(str(input_path), str(output_file), args.stopwords):
            return 1
    else:
        # Директория
        tokenized_files = []
        for file_path in input_path.glob("*.txt"):
            print(f"Processing {file_path.name}...")
            output_file = output_dir / f"{file_path.stem}_tokens.txt"
            if run_tokenizer(str(file_path), str(output_file), args.stopwords):
                tokenized_files.append(str(output_file))
        
        print(f"\nTokenized {len(tokenized_files)} files")
    
    print("\nIndex building completed!")
    return 0

if __name__ == '__main__':
    sys.exit(main())