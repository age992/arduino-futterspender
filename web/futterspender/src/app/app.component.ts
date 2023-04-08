import { Component } from '@angular/core';
import { RouterOutlet } from '@angular/router';

import { slider } from '../animations/Slider';

@Component({
  selector: 'app-root',
  animations: [slider],
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.scss'],
})
export class AppComponent {
  title = 'futterspender';

  getRouteAnimationData(outlet: RouterOutlet) {
    return outlet?.activatedRouteData['animation'];
  }
}
