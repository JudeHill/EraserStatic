echo "hi"
eraser_static . output_barrier.txt --write-all --no-llms
eraser_static . output_vanilla.txt -b --write-all --no-llms
diff output.txt output_b.txt
echo "Diff is shown above"