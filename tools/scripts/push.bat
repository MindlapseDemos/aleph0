del ddsrc.zip
zip -r ddsrc.zip . -i *.c -i *.h -i *.asm -i *.s -i *.S -i *.inl -i *akefile* -i *.bat -i packsrc -i unpsrc -x *.swp -x libs/*
scp2dos -g ddsrc.zip nuclear@192.168.0.4:code/demoscene/dosdemo
