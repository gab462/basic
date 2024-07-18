async function main() {
    let wasm = await WebAssembly.instantiateStreaming(fetch('index.wasm'), {
        env: {
            write: (_, ptr, len) => console.log(
                new TextDecoder()
                    .decode(
                        new Uint8Array(
                            wasm.instance.exports.memory.buffer, ptr, len))),
            assert_here: (file, len, line, cond) => { if (!cond) throw new Error(`${cstr(file, len)}:${line}: Assertion Fail`); },
        }
    });

    wasm.instance.exports.init();
}

main();
