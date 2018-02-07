# paperback-cli
Paperback-cli is the crossplatform, backwards-compatible, command line version of Oleh Yuschuk's [PaperBack](http://www.ollydbg.de/Paperbak/index.html). Originally designed to print encoded data directly to paper, it has been simplified to focus on encoding and decoding bitmaps that can be written to any printable media via whatever printing mechanism is available to your system. Recommended for small files such as cryptocurrency wallets, key revocation certificates, password databases, and any other important data only a few megabytes large. While the capacity is small compared to conventional storage media, this program encodes or decodes non-electronic backups characteristically resilient to or unaffected by electromagnetic disturbance, water, and heat.

#### Build Requirements
* No external dependencies

#### Building
```bash
        git submodule update --init --recursive
        make
```


#### Encode arbitrary data to bitmap 
[Symmetric encryption](http://www.tutonics.com/2012/11/gpg-encryption-guide-part-4-symmetric.html) and compression recommended prior to encoding
```bash
        ./paperback-cli --encode -i [input] -o [output].bmp
```

#### Decode encoded bitmap
```bash
        ./paperback-cli --decode -i scanned.bmp -o original.gpg
```

#### Decode multiple encoded bitmaps
e.g. scanned_0001.bmp through scanned_0029.bmp
```bash 
        ./paperback-cli --decode -i scanned.bmp -o original.gpg -p [nPages]
```


#### List all arguments and settings
```bash
        ./paperback-cli --help
```


#### Compiler(s)
* mingw-w64, GCC 6.3.0 under msys2
* gcc version 7.1.1 20170630


#### What has changed from 1.1?
Decryption and decompression has been ported for backwards compatibility with existing backups but more appropriate tools, such as gpg, tar, and bzip2, should be used to preprocess the data before encoding. Printing has been removed entirely for cross-platform compatibility.


#### What settings should I use?
Settings depend on the target printer, scanner, and the abuse you expect the printed medium to endure. Inkjet printers are substantially less precise, requiring low DPI settings to be readable (200 by default). The recommended DPI for laser printers is half of your scanner DPI (600 dpi printing possible with scans of 1200 DPI).  Oleh recommends a dot size of 70% (default) to ensure adequate white space.  Redundancy guards against partially damaged data (default of 5 is 1 block of redundant data per 5 blocks), so higher settings increase chances of recovery after damage.  Header and footer prints page and file information (on by default) but has not yet been implemented in this version.  Border prints a black border around the data (disabled by default to save ink).

#### Similar projects:
Several QR code-based paper backup programs have been written since Oleh released PaperBack 1.1, each with their own advantages.  Intra2net ([paperbackup](https://github.com/intra2net/paperbackup)) explains that the ubiquity of QR codes allows his solution several high-quality encoder/decoders.  With good density, excellent error-correction, and several alternatives if one decoder should fail, QR codes are an excellent choice.  PaperBack also detects and repairs damaged data but the advantage of PaperBack is signifcantly higher density, due to the layout of data.  Every cell of a QR code sacrifices space for alignment blocks, whereas PaperBack uses the grid itself for this information.  The disadvantage is, should PaperBack fail to decode, there are no alternatives except previous versions of PaperBack.  Twibright's [Optar](http://ronja.twibright.com/optar/) is very similar to PaperBack, but, according to Oleh's claims, PaperBack stores 500kB per page while Twibright claims to store 200kB.  Comparision testing to follow as time allows.

__________________

Forked from [Oleh Yuschuk's PaperBack](http://www.ollydbg.de/Paperbak/index.html)

Paperback-cli is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

