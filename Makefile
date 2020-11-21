default: program

obj/%.o: %.S
	@mkdir -p $(dir $(@))
	cc -g -c -o $@ $^

obj/%.o: %.c
	@mkdir -p $(dir $(@))
	cc -g -c -o $@ $^


program: obj/main.o obj/sandbox.o
	cc -o $@ $^

clean:
	rm -f obj program
