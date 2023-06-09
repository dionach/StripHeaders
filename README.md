StripHeaders
============

A Native-Code module for IIS 7.0 and above, designed to easily remove unnecessary response headers and prevent information leakage of software and version information, which can be useful to an attacker.

For more information, and binary releases, please see the related blog post:

[http://www.dionach.com/blog/easily-remove-unwanted-http-headers-in-iis-70-to-85](https://www.dionach.com/blog/easily-remove-unwanted-http-headers-in-iis-7-0-to-8-5/)

Installation
============

Download the latest MSI installer from the release page (https://github.com/Dionach/StripHeaders/releases) and run the installer on your IIS servers. 

Alternatively, use Group Policy to assign the installer package to all IIS servers within an organisation. See http://support.microsoft.com/kb/816102 for more information.

Configuration
=============

The StripHeaders module uses a very simple syntax to add additional headers to remove. The configuration is contained in the applicationHost.config file and can be edited directly or from the Configuration Manager module in the site settings in IIS Manager. The default configuration is shown below:
```xml
<configuration> [...]
  <system.webServer> [...]
    <stripHeaders>
      <header name="Server" />
      <header name="X-Powered-By" />
      <header name="X-Aspnet-Version" />
    </stripHeaders>
  </system.webServer>
</configuration>
```
