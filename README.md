# Enhanced Static Approximation to the Eraser Lockset Algorithm

This project develops a **static approximation of the Eraser Lockset algorithm** for detecting data races in multithreaded programs.  
It extends the original Eraser approach to **account for memory barriers**, reducing false positives in race detection.

The project also explores the **use of large language models (LLMs)** to interpret concurrency intent — identifying which synchronization mechanisms protect shared variables, explaining race causes, and potentially replicating Eraser’s functionality through prompt-based analysis.

Benchmarks used include **SPLASH** and **DataRaceBench**, with additional **synthetic tests** for evaluating AI-based detection.
