ssh2dos -g -i c:\prog\ssh2dos\doskey -S -B nuclear 192.168.0.4 cd code/demoscene/dosdemo && git archive -o dd.zip master
scp2dos -r -g -i c:\prog\ssh2dos\doskey nuclear@192.168.0.4:code/demoscene/dosdemo/dd.zip .
unzip32 -o dd.zip
del dd.zip
