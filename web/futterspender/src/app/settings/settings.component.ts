import { Component } from '@angular/core';
import { Location } from '@angular/common';

@Component({
  selector: 'settings',
  templateUrl: './settings.component.html',
  styleUrls: ['./settings.component.scss'],
})
export class SettingsComponent {
  constructor(private location: Location) {}

  navBack() {
    this.location.back();
  }

  save() {
    this.navBack();
  }
}
