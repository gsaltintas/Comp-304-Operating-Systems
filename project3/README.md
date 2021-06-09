# Project 3
## Team
İdil Defne Çekin,icekin17, 64387
Gül Sena Altıntaş, galtintas17, 64284

## Run
We provided a Makefile to run the project, simple execute the following command from your terminal
```bash
make all
```
To run the parts do following:
- Part 1
    ```bash
    ./part1  BACKING_STORE.bin address.txt
    ```
- Part 2 has a command line argument `-p`, if you would like to run FIFO pass `-p 0`, `-p 1` for LRU.
    ```bash
    ./part2  BACKING_STORE.bin address.txt -p 0|1
    ```

# Part 1
## Address Translation
The addresses provided in the `addresses.txt` file are in decimal format while we need the binary format to process the 20 bits of the virtual addresses. We shortcut the decimal-binary-decimal conversion. Since in the binary-decimal conversion the least significant 10 bits would convert to 1024, we find offset as `logical_address % 1024` and page number as `logical_address//1024`.

## TLB
### Search
We go over all the entries in the `tlb` and return the corresponding struct's `physical` value, otherwise return `-1`.

### Addition
Write the new_entry to the next location in `tlb` using the counter `tlbindex`