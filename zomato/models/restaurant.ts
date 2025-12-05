import { MenuItem } from "./menu-item";

export class Restaurant {
  private id: number;
  private name: string;
  private location: string;
  private menu: MenuItem[];

  constructor(id: number, name: string, location: string, menu: MenuItem[]) {
    this.id = id; //auto generated
    this.name = name;
    this.location = location;
    this.menu = menu;
  }

  getName(): string {
    return this.name;
  }

  getLocation(): string {
    return this.location;
  }

  getMenu(): MenuItem[] {
    return this.menu;
  }

  setName(name: string): void {
    this.name = name;
  }

  setLocation(location: string): void {
    this.location = location;
  }

  setMenu(menu: MenuItem[]): void {
    this.menu = menu;
  }
}
