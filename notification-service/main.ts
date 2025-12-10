//Singleton Pattern Implementation for Notification Service
//Decorator Pattern Implementation for Notification Content
//Observer Pattern Implementation for Notification Subscribers


interface INotification {
    getContent(): string;
}

class SimpleNotification implements INotification {
    private text: string;
    constructor(text: string) {
        this.text = text;
    }
    getContent(): string {
        return this.text;
    }
}


//Decorator Pattern Implementations
abstract class INotificationDecorator implements INotification {
    constructor(protected inner: INotification) { }
    abstract getContent(): string;
}

class TimestampDecorator extends INotificationDecorator {
    constructor(inner: INotification) {
        super(inner);
    }
    getContent(): string {
        return `[${new Date().toISOString()}] ${this.inner.getContent()}`;
    }
}

class SignatureDecorator extends INotificationDecorator {
    constructor(inner: INotification, private signature: string) {
        super(inner);
    }
    getContent(): string {
        return `${this.inner.getContent()} \n-- ${this.signature}`;
    }
}



//Observer Pattern Implementations
interface IObserver {
    update(): void;
}

interface IObservable {
    addObserver(observer: IObserver): void;
    removeObserver(observer: IObserver): void;
    notifyObservers(): void;
}

//Concrete Observable
class NotificationObservable implements IObservable {
    private observers: IObserver[] = [];
    private currentNotification: INotification | null = null;

    addObserver(observer: IObserver): void {
        this.observers.push(observer);
    }
    removeObserver(observer: IObserver): void {
        this.observers = this.observers.filter(obs => obs !== observer);
    }

    notifyObservers(data?: any): void {
        for (const observer of this.observers) {
            observer.update();
        }
    }
    setNotification(notification: INotification): void {
        this.currentNotification = notification;
        this.notifyObservers(notification);
    }
    getNotification(): string | null {
        return this.currentNotification.getContent();
    }
}

//Concrete Observer
class Logger implements IObserver {
    private observable: NotificationObservable;
    constructor(observable: NotificationObservable) {
        this.observable = observable;
    }

    update(): void {
        console.log("Logger: New Notification - " + this.observable.getNotification());
    }
}


//Strategy Pattern Implementation for Notification Sending
interface INotificationStrategy {
    send(notification: string): void;
}

class EmailNotificationStrategy implements INotificationStrategy {
    private emailAddress: string;
    constructor(email: string) {
        this.emailAddress = email;
    }

    send(notification: string): void {
        console.log("Sending Email Notification: " + notification + " to " + this.emailAddress);
    }
}

class SMSNotificationStrategy implements INotificationStrategy {
    private phoneNumber: string;
    constructor(phone: string) {
        this.phoneNumber = phone;
    }
    send(notification: string): void {
        console.log("Sending SMS Notification: " + notification + " to " + this.phoneNumber);
    }
}

class PopUpNotificationStrategy implements INotificationStrategy {
    send(notification: string): void {
        console.log("Showing Pop-Up Notification: " + notification);
    }
}

class NotificationEngineObserver implements IObserver {
    private observable: NotificationObservable;
    private strategies: INotificationStrategy[] = [];

    constructor(observable: NotificationObservable) {
        this.observable = observable;
    }

    addStrategy(strategy: INotificationStrategy): void {
        this.strategies.push(strategy);
    }

    update(): void {
        const notificationContent = this.observable.getNotification();

        for (const strategy of this.strategies) {
            strategy.send(notificationContent);
        }
    }

}



//Singleton Notification Service
class NotificationService {
    private static instance: NotificationService;
    private observable: NotificationObservable;
    private notifications: INotification[] = [];  // Store notifications

    private constructor() {
        this.observable = new NotificationObservable();
    }

    static getInstance(): NotificationService {
        if (!NotificationService.instance) {
            NotificationService.instance = new NotificationService();
        }
        return NotificationService.instance;
    }

    getObservable(): NotificationObservable {
        return this.observable;
    }

    sendNotification(notification: INotification): void {
        this.notifications.push(notification);
        this.observable.setNotification(notification);
    }
}

//Usage Example
const notificationService = NotificationService.getInstance();
const observable = notificationService.getObservable();

const logger = new Logger(observable);
observable.addObserver(logger);

const notificationEngine = new NotificationEngineObserver(observable);
notificationEngine.addStrategy(new EmailNotificationStrategy("abc@gmail.com"));
notificationEngine.addStrategy(new SMSNotificationStrategy("123-456-7890"));
notificationEngine.addStrategy(new PopUpNotificationStrategy());
observable.addObserver(notificationEngine);

//Creating a notification with decorators
let notification: INotification = new SimpleNotification("Your order has been shipped!");
notification = new TimestampDecorator(notification);
notification = new SignatureDecorator(notification, "Best Regards, Shop Team");

notificationService.sendNotification(notification);

//Sending another notification
let anotherNotification: INotification = new SimpleNotification("Your package has been delivered!");
anotherNotification = new TimestampDecorator(anotherNotification);
anotherNotification = new SignatureDecorator(anotherNotification, "Best Regards, Shop Team");
notificationService.sendNotification(anotherNotification);