<!--
*** Thanks for checking out the Best-README-Template. If you have a suggestion
*** that would make this better, please fork the repo and create a pull request
*** or simply open an issue with the tag "enhancement".
*** Thanks again! Now go create something AMAZING! :D
***
***
***
*** To avoid retyping too much info. Do a search and replace for the following:
*** ZwCreatePhoton, BrowserPivotingIE, @ZwCreatePhoton, email, BrowserPivotingIE, Browser Pivoting implementation for Internet Explorer
-->



<!-- PROJECT SHIELDS -->
<!--
*** I'm using markdown "reference style" links for readability.
*** Reference links are enclosed in brackets [ ] instead of parentheses ( ).
*** See the bottom of this document for the declaration of the reference variables
*** for contributors-url, forks-url, etc. This is an optional, concise syntax you may use.
*** https://www.markdownguide.org/basic-syntax/#reference-style-links
-->
<!--
[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![MIT License][license-shield]][license-url]
[![LinkedIn][linkedin-shield]][linkedin-url]
-->


<!-- PROJECT LOGO -->
<br />
<p align="center">
  <a href="https://github.com/ZwCreatePhoton/BrowserPivotingIE">
<!--    <img src="images/logo.png" alt="Logo" width="80" height="80"> -->
  </a>

  <h3 align="center">BrowserPivotingIE</h3>

  <p align="center">
    Browser Pivoting implementation for Internet Explorer
    <br />
<!--    <a href="https://github.com/ZwCreatePhoton/BrowserPivotingIE"><strong>Explore the docs »</strong></a> -->
    <br />
    <br />
    <!--
    <a href="https://github.com/ZwCreatePhoton/BrowserPivotingIE">View Demo</a>
    ·
    -->
    <a href="https://github.com/ZwCreatePhoton/BrowserPivotingIE/issues">Report Bug</a>
    ·
    <a href="https://github.com/ZwCreatePhoton/BrowserPivotingIE/issues">Request Feature</a>
  </p>
</p>



<!-- TABLE OF CONTENTS -->
<details open="open">
  <summary><h2 style="display: inline-block">Table of Contents</h2></summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About The Project

<!--
[![Product Name Screen Shot][product-screenshot]](https://example.com)
-->

BrowserPivotingIE implements the browser pivoting technique for Internet Explorer. Additional information about the technique can be found [here.](https://www.cobaltstrike.com/help-browser-pivoting)

This repo is split into multiple parts:


**[HTTPParser](BrowserPivotingIE/HTTPParser)**

Static library project that handles the parsing of HTTP messages

**[HTTPProxy](BrowserPivotingIE/HTTPProxy)**

DLL project that implements that the meat of the technique. Upon being loaded, the DLL will host an HTTP proxy on port 8080. Requests will be sent to the destination using WinINET APIs. As a consequence, the request will inherit the browser's authentication credentials, cookies, etc.

**[HTTPProxyHost](BrowserPivotingIE/HTTPProxyHost)**

EXE project that hosts the HTTPProxy DLL.

**[InjectIE](BrowserPivotingIE/InjectIE)**

EXE project that embeds the HTTPProxy DLL as a resource and injects the DLL into Internet Explorer using the DLL Injection technique. 

**[WinINETPOC](BrowserPivotingIE/BasicAuthHTTPServer.py)**

A simple example on how WinINet persists credentials accross requests on a per process basis.

**[BasicAuthHTTPServer.py](BrowserPivotingIE/BasicAuthHTTPServer.py)**

A Python3 based HTTP server that supports [Basic Authentication](https://en.wikipedia.org/wiki/Basic_access_authentication).

<!-- 
### Built With

* []()
* []()
* []()

-->



<!-- GETTING STARTED -->
## Getting Started

To get started with the Usage example follow these simple steps.

### Prerequisites

* Visual Studio (optional)


### Installation

1. Compile the "InjectIE" project with Visual Studio (optional if using the release binary). The build architecture (x86 vs x64) must match the architecture of the target Internet Explorer process. 

<!-- USAGE EXAMPLES -->
## Usage

1. Host or identify a website with authentication (NTLM, cookies, etc). This example will use a server hosted at 127.0.0.1:80 using [_BasicAuthHTTPServer.py_](BrowserPivotingIE/BasicAuthHTTPServer.py).
```sh
python3 BasicAuthHTTPServer.py 80
```

2. Open an x86 instance of Internet Explorer
3. In Internet Explorer, browse to http://127.0.0.1 and authenticate using the credentials `demo:demo`. Upon authenticating, the page will present this text
```{"path": "/", "get_vars": "{}"}```
4. Open an instance of Firefox.
5. In Firefox, browse to http://127.0.0.1 and note the prompt for credentials.
6. Run InjectIE.exe on the target machine. InjectIE.exe will inject into iexplore.exe a DLL that implements an HTTP proxy server on port 8080.
7. Configure Firefox to the reach the internet via the HTTP proxy server 127.0.0.1:8080. [If the proxy is on localhost, the `network.proxy.allow_hijacking_localhost` config setting will need to be modified.](https://stackoverflow.com/questions/57419408/how-to-make-firefox-use-a-proxy-server-for-localhost-connections)
8. In Firefox, browse to http://127.0.0.1 and note that the session is authenticated.

<!-- ROADMAP -->
## Roadmap

See the [open issues](https://github.com/ZwCreatePhoton/BrowserPivotingIE/issues) for a list of proposed features (and known issues).



<!-- CONTRIBUTING -->
## Contributing

Contributions are what make the open source community such an amazing place to be learn, inspire, and create. Any contributions you make are **greatly appreciated**.

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request



<!-- LICENSE -->
## License

Distributed under the MIT License. See `LICENSE` for more information.



<!-- CONTACT -->
## Contact

ZwCreatePhoton - [@ZwCreatePhoton](https://twitter.com/ZwCreatePhoton)

Project Link: [https://github.com/ZwCreatePhoton/BrowserPivotingIE](https://github.com/ZwCreatePhoton/BrowserPivotingIE)



<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[contributors-shield]: https://img.shields.io/github/contributors/ZwCreatePhoton/repo.svg?style=for-the-badge
[contributors-url]: https://github.com/ZwCreatePhoton/repo/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/ZwCreatePhoton/repo.svg?style=for-the-badge
[forks-url]: https://github.com/ZwCreatePhoton/repo/network/members
[stars-shield]: https://img.shields.io/github/stars/ZwCreatePhoton/repo.svg?style=for-the-badge
[stars-url]: https://github.com/ZwCreatePhoton/repo/stargazers
[issues-shield]: https://img.shields.io/github/issues/ZwCreatePhoton/repo.svg?style=for-the-badge
[issues-url]: https://github.com/ZwCreatePhoton/repo/issues
[license-shield]: https://img.shields.io/github/license/ZwCreatePhoton/repo.svg?style=for-the-badge
[license-url]: https://github.com/ZwCreatePhoton/repo/blob/master/LICENSE.txt
[linkedin-shield]: https://img.shields.io/badge/-LinkedIn-black.svg?style=for-the-badge&logo=linkedin&colorB=555
[linkedin-url]: https://linkedin.com/in/ZwCreatePhoton
