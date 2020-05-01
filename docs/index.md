XPMP2 - Developer's documentation
===

These pages, available as [GitHub pages](https://twinfan.github.io/XPMP2/),
document both usage and internals of the XPMP2 library.

**This is work in progress!**

- [ ] Doxgen-generated code documentation
- [ ] File format for `xsb_aircraft.txt` (this is currently scattered across [kuroneko's wiki](https://github.com/kuroneko/libxplanemp/wiki))
- [ ] dataRefs supported by XPMP2 (this is partly at [kuroneko's](https://github.com/kuroneko/libxplanemp/wiki/OBJ8-CSL#animations), partly at [my fork](https://github.com/TwinFan/libxplanemp/wiki/OBJ8-CSL-dataRefs))
- [ ] Provide some general "how it works" background, can base on kuronekos wiki (but shorter) and on some of [my additions](https://github.com/TwinFan/libxplanemp/wiki#changes-to-multiplayeraitcas-handling)
    - [ ] Startup sequence
    - [ ] Model Matching (see [LT FAQ](https://twinfan.gitbook.io/livetraffic/reference/faq#matching))
    - [ ] TCAS support (again see [LT documentation](https://twinfan.gitbook.io/livetraffic/introduction/features/tcas))
    - [ ] AI/multiplayer support ([LT has a bit](https://twinfan.gitbook.io/livetraffic/introduction/features/api#standard-multiplayer-datarefs))

### API and Code Documentation ###

All header (and code) files are documented using
[Doxygen](http://www.doxygen.nl/)-style comments.
The generated doxygen files are checked in, too, so that the are available
online:

- [Main Page](https://twinfan.github.io/XPMP2/html/index.html)


Links to outside locations:
--

Original libxplanemp:
- [TwinFan's libxplanemp fork](https://github.com/TwinFan/libxplanemp) on GitHub
    - [wiki explaining differences to the kuroneko fork](https://github.com/TwinFan/libxplanemp/wiki)
- [kuroneko's fork](https://github.com/kuroneko/libxplanemp) on GitHub, a long-time standard and basis of other important forks
    - [kuroneko's wiki](https://github.com/kuroneko/libxplanemp/wiki) including notes on CSL development
