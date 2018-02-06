char MAIN_page[] PROGMEM = R"=====(<!DOCTYPE html>
<html>
<style>
img {height: 64px; width: 64px}
html, body {height: 100%; width: 100%; font-size: 20pt;}
</style>
<body>
<h2>ESPFridge</h2>
Temp: {{temp}}
<br/>
Target Temp: <input id='targetTemp' type="number" max="80" min="40" value="{{targetTemp}}" style='font-size: 20pt'><div style='display:inline'>
<img onclick="document.getElementById('targetTemp').stepUp(); document.getElementById('tempSetter').src='/settemp?temp='+document.getElementById('targetTemp').value" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAABHNCSVQICAgIfAhkiAAAAAlwSFlzAAADdgAAA3YBfdWCzAAAABl0RVh0U29mdHdhcmUAd3d3Lmlua3NjYXBlLm9yZ5vuPBoAAAMcSURBVHja7dffaxxVFAfw7/feO3dmdmYTs2k2DRo2IVVsYyNR+6AGZUFKI7ZWY1MR+ty0TwoBbSkREQSfBUGoFMS3QhUs9klQ/wqhxQftg0iRWrO63dl7r/Mj7A9LKNU0Qdhz+LLzdj6cObAMnXPYyRJpBoAB4P8LYJ2ifrKudgTANzg99lj12uSB2vXqavXhbQVwNzUEv9wlR6d/vv7T+I31377gLPW2AfA8P69ElbkYMb7/5jto6Fk8x4vbAuAyz4RR+Nq+sb2wnoULHcwvCbySf5in5Xv3FcBXuCh9tfbk1BOkJpx2YEy0ZRvyJqmGvHe4oo7eFwCPcQoeLhyYeSqI4ghGGRjPQMYSKAFN8xfapqUxgk+5wpktBeQHluDS/un94xMTE7DS5uu3yhaACEWaDjCugoq6zBfpbxkAM/is9mBtfm52Dlbkg4toC5YIlNBBKE+gHMWP4nF5aUsAfIlnKkOV5foz9Xy4kSZLDnCeg/VtBugg2mEbiWwiLJcW+bb84D8BeJiLWuu1IwePUPoyAxSRNr8BBoQNbN8GEAPNoAkVk3o4WOWqWvpXAL7KKRAXlg4uBeWRMmzWzAGdDVATiU6AED2IIn/4tzC0K/ZYEef5JvfcA6B7dIeePjRem6nlgw0N0i4Q0nYOEQEyQD8iLnJD/woX2AdQ9a5sdpRis6Obf2R+fuHZBRhnYJ2FxQZCmM5rcMoBPoCgF9AfrypQGR7ZgwX11V0A3aObHJtcPv7ycSQuyYZ3kje7ECccoNGL6G4gKkBJkOD2UANxVH6B73sfbgLoHl0pLK2dev0UqQlrLYw1xRaK7tsGJACVxkuj+xBFwgLXUA14ZTKIwrf4rjqGnlK9/+2I8cnZE2eD0d2jWP9zHVpq+MJHS7YQpO3oQEWItGXWfpqWBD3mCPoESyx+DQECwgigDZgkwdToQ94PuPYRgIt3APAt3MrHK7z641WcO39uQ5XG/0eyYWpjmBKgFIAgoNJIFs9CACwAoAQcYEA0/N+z51voqb7vAi5ToIFhaGx9tVDU17jpeoYOPkwGgAFgxwF/A9z/FVdEAW5kAAAAAElFTkSuQmCC">
<img onclick="document.getElementById('targetTemp').stepDown(); document.getElementById('tempSetter').src='/settemp?temp='+document.getElementById('targetTemp').value" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAABHNCSVQICAgIfAhkiAAAAAlwSFlzAAADdgAAA3YBfdWCzAAAABl0RVh0U29mdHdhcmUAd3d3Lmlua3NjYXBlLm9yZ5vuPBoAAANrSURBVHja7ZfdaxxVGMafc87uJmlTEwspRa/qhXjhlR/0TxAEsUjbmyKUouh/4IWotZQ21EKvbPDaG0n6QRuhH7QIxY9AsVkIsR+alBjSJk2yO5mdMzNnzjnzejJnsVujpWZJvXD35QfvDLvzPHPmmXPOMiLCf/nhjo6BjoH/t4FS6wH7gAk8wDMbqrgNIX1Fdo0BtpdxpKgCePmhI0fXo7AKBysx3wvfM+4ocUAAjDF/Hsz33PXECuCI8savAF5cOwKLji3oP/zeYQzsGEAUR4hVjCiN0MgaiG2MxCSIXUlXEUUQ3QLXG9eBzQzYRA40+xwwDGi0EDK8VHkBUi1t/dsM0HeUg/D+ka+PpMvzyzBkkOUZVK6Q2QypSRHpCGEWIkgD1LM66qoOYw201avfKVBGIVUpksyZ1c6slsXvKtSFiXvT+peZ+Q//MYQ0ShfjJD409M0Q5VkOzjkEExBcgK8WdzRLuAIByB3WYRzakTpkE+XP9ea90AFRKpMT9Kk59di3gL6lo7OLsyPD54dRRtmLMk+ruMgFmGVeOCvw4nGLgQQoZ2V013oRypUr9In+6Mlew9/x7vid8eq1H675u/cGvDC5Y+IFLGePiidNA5FD+n7rygCWastTGDNvPfE8QFXK0IVdl3+6vHD3t7tenAp5cPI9txxcc0D9RVw+NLBNbseD+UUXGP0GnSf1ryYiGqEZEPafuXQmDZdDP/jUxHpIUzHESFoNePqSfgT3Qk1BfoCO09Q6ZkIfSq3156OXRskqC55zhxcXRoAUgSu+Rrwn7YFZIsrC9As6Zs62NRU7E4O1sDZy9furq+JF+IQV4IaDZcwbaBEvxSVUZA/kSnSBBu3H7a8FzVDOzs1WqxNVcMs92kMxeQNNE1uoDyuN8DYm7TvtL0atoSxj1+T05MLc/Tlwwz2aw0r7p3hfuR/1KKghMG/60LVnYG0oNfb/fPtGKhuyeAxCC9jIFuKb+GZInWQIcIC+pOkNWY7pLF20yhy6MTVOpKjIAEWESl6B7SYyoR6kIXNuQ/cDNExHYxmfvnn/VjECJVWCeLYMJZNROmk/ezobklu0rxbVJiKSeOW1V2HJ3sSPtOcp7Yh8KLt45e1ABzPPbX9+YeeO13cXQV3XxYjWzcHhg6WxO2M97Vyj88+oY6Bj4A/5Q1lRvUR0ZwAAAABJRU5ErkJggg==">
</div>
<br/>
<br/>
<a href="/reboot">Reboot</a>
<img id='tempSetter' src='/foo' style='height: 0; width: 0;'/>
</body>
</html>
)=====";
