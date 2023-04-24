import { ELanguage } from 'src/app/lib/ELanguage';
import { ETheme } from 'src/app/lib/ETheme';
import { Settings } from 'src/models/Settings';

export const settings: Settings = {
  Language: ELanguage.German,
  Theme: ETheme.Boring,
  Notifications: {
    ContainerEmpty: { Active: false, Email: false, Phone: false },
    DidNotEatInADay: { Active: false, Email: false, Phone: false },
  },
};
