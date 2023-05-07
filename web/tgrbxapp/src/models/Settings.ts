import { ELanguage } from 'src/lib/ELanguage';
import { ETheme } from 'src/lib/ETheme';
import { Notification } from './Notification';

export interface Settings {
  PetName?: String;
  PlateTAR?: Number;
  PlateFilling?: Number;
  Notifications: {
    ContainerEmpty: Notification;
    DidNotEatInADay: Notification;
  };
  Email?: String;
  Phone?: String;
  Language: ELanguage;
  Theme: ETheme;
}
