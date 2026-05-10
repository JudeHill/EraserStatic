import subprocess
import os
import time

files = [
    "demo_prog.c",
    "two_diff_locks_complex.c",
    "two_stage_swap_2.c",
    "wait_for_main_complex.c",
    "write_then_read_complex.c",
    "test_script.py",
    "two_phase_read_write.c",
    "two_stage_swap_2_complex.c",
    "write_then_read.c",
    "two_diff_locks.c",
    "two_stage_swap.c",
    "wait_for_main.c",
    "write_then_read_2.c",
]

def run_parameterized_command(filenames, opts, *, delay=True):
    for filename in filenames:
        output_filename = f"{filename}_fps.log"
        
        print(f"Processing {filename} -> {output_filename}...")
        
        # 1. Split the command into a list of individual arguments
        cmd = [
            "eraser_static", 
            filename, 
            output_filename, 
        ]
        if opts:
            for o in opts:
                cmd.append(o)
        
        try:
            # 2. Execute the command
            # Using a list is safer and prevents the FileNotFoundError
            subprocess.run(
                cmd, 
                stderr=subprocess.PIPE, 
                text=True,
                check=True
            )
            print(f"Successfully processed {filename}")
            
        except FileNotFoundError:
            print(f"Error: The executable 'eraser_static' was not found in your PATH.")
            break # Stop early if the tool itself is missing
        except subprocess.CalledProcessError as e:
            print(f"Error running command on {filename}: {e.stderr}")
        if delay:
            print("Request sent. Waiting 60 seconds to avoid rate limits...")
            time.sleep(60) 
            print("Resuming...")

run_parameterized_command(files, ["--eval-fps", "--llm=gemini"], delay=False)