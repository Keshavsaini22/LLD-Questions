// npx tsx book-my-show/index.ts

// ================= ENUMS =================

enum SeatCategory {
  SILVER = "SILVER",
  GOLD = "GOLD",
  PLATINUM = "PLATINUM",
}
enum SeatStatus {
  AVAILABLE = "AVAILABLE",
  LOCKED = "LOCKED",
  BOOKED = "BOOKED",
}
enum BookingStatus {
  CONFIRMED = "CONFIRMED",
  CANCELLED = "CANCELLED",
}
enum PaymentStatus {
  SUCCESS = "SUCCESS",
  FAILED = "FAILED",
}
enum ScreenType {
  TWO_D = "2D",
  THREE_D = "3D",
  FOUR_D = "4DX",
  IMAX = "IMAX",
}

// ================= ENTITIES =================

class City {
  constructor(
    public id: string,
    public name: string,
  ) {}
}

class Movie {
  constructor(
    public id: string,
    public name: string,
    public isPremium: boolean = false,
  ) {}
}

class User {
  constructor(
    public id: string,
    public name: string,
    public bookings: Booking[] = [],
  ) {}
}

class Seat {
  constructor(
    public seatNumber: string,
    public category: SeatCategory,
  ) {}
}

class Screen {
  constructor(
    public id: string,
    public name: string,
    public type: ScreenType,
    public seats: Seat[],
  ) {}
}

class Theatre {
  constructor(
    public id: string,
    public name: string,
    public city: City,
    public screens: Screen[] = [],
  ) {}

  addScreen(screen: Screen) {
    this.screens.push(screen);
  }
}

// ================= PRICING STRATEGY =================

interface PricingStrategy {
  calculate(movie: Movie, screen: Screen, category: SeatCategory): number;
}

class StandardPricingStrategy implements PricingStrategy {
  calculate(movie: Movie, screen: Screen, category: SeatCategory): number {
    let base = 0;

    switch (category) {
      case SeatCategory.SILVER:
        base = 200;
        break;
      case SeatCategory.GOLD:
        base = 300;
        break;
      case SeatCategory.PLATINUM:
        base = 400;
        break;
    }

    let multiplier = 1;

    switch (screen.type) {
      case ScreenType.THREE_D:
        multiplier += 0.3;
        break;
      case ScreenType.FOUR_D:
        multiplier += 0.6;
        break;
      case ScreenType.IMAX:
        multiplier += 0.8;
        break;
    }

    if (movie.isPremium) multiplier += 0.2;

    return base * multiplier;
  }
}

// ================= SHOW (WITH 5-MIN LOCK) =================

class Show {
  private seatStatus = new Map<string, SeatStatus>();

  private seatLockInfo = new Map<string, { userId: string; lockedAt: number; timeoutRef: any }>();

  private static LOCK_DURATION = 5 * 60 * 1000; // 5 minutes

  constructor(
    public id: string,
    public movie: Movie,
    public theatre: Theatre,
    public screen: Screen,
    public time: Date,
    private pricing: PricingStrategy,
  ) {
    for (let seat of screen.seats) {
      this.seatStatus.set(seat.seatNumber, SeatStatus.AVAILABLE);
    }
  }

  lockSeats(seats: string[], userId: string) {
    const now = Date.now();

    for (let seat of seats) {
      const status = this.seatStatus.get(seat);

      if (status === SeatStatus.LOCKED) {
        const lockInfo = this.seatLockInfo.get(seat)!;

        if (now - lockInfo.lockedAt > Show.LOCK_DURATION) {
          this.forceReleaseSeat(seat);
        } else {
          throw new Error(`Seat already locked: ${seat}`);
        }
      }

      if (this.seatStatus.get(seat) !== SeatStatus.AVAILABLE) {
        throw new Error(`Seat not available: ${seat}`);
      }
    }

    for (let seat of seats) {
      this.seatStatus.set(seat, SeatStatus.LOCKED);

      const timeoutRef = setTimeout(() => {
        this.autoReleaseSeat(seat);
      }, Show.LOCK_DURATION);

      this.seatLockInfo.set(seat, {
        userId,
        lockedAt: now,
        timeoutRef,
      });
    }
  }

  private autoReleaseSeat(seat: string) {
    const lockInfo = this.seatLockInfo.get(seat);
    if (!lockInfo) return;

    this.seatStatus.set(seat, SeatStatus.AVAILABLE);
    this.seatLockInfo.delete(seat);

    console.log(`Seat ${seat} auto-released after 5 mins`);
  }

  private forceReleaseSeat(seat: string) {
    const lockInfo = this.seatLockInfo.get(seat);
    if (!lockInfo) return;

    clearTimeout(lockInfo.timeoutRef);
    this.seatStatus.set(seat, SeatStatus.AVAILABLE);
    this.seatLockInfo.delete(seat);
  }

  releaseSeats(seats: string[], userId: string) {
    for (let seat of seats) {
      const lockInfo = this.seatLockInfo.get(seat);

      if (lockInfo && lockInfo.userId === userId) {
        clearTimeout(lockInfo.timeoutRef);
        this.seatStatus.set(seat, SeatStatus.AVAILABLE);
        this.seatLockInfo.delete(seat);
      }
    }
  }

  confirmSeats(seats: string[]) {
    for (let seat of seats) {
      const lockInfo = this.seatLockInfo.get(seat);
      if (lockInfo) {
        clearTimeout(lockInfo.timeoutRef);
        this.seatLockInfo.delete(seat);
      }

      this.seatStatus.set(seat, SeatStatus.BOOKED);
    }
  }

  getSeatPrice(seatNumber: string): number {
    const seat = this.screen.seats.find((s) => s.seatNumber === seatNumber);
    if (!seat) throw new Error("Invalid seat");
    return this.pricing.calculate(this.movie, this.screen, seat.category);
  }

  getSeatMap() {
    return this.seatStatus;
  }
}

// ================= PAYMENT STRATEGY =================

interface PaymentStrategy {
  pay(amount: number): PaymentStatus;
}

class UPIPayment implements PaymentStrategy {
  pay(amount: number): PaymentStatus {
    console.log("Paid via UPI:", amount);
    return PaymentStatus.SUCCESS;
  }
}

class CreditCardPayment implements PaymentStrategy {
  pay(amount: number): PaymentStatus {
    console.log("Paid via Credit Card:", amount);
    return PaymentStatus.SUCCESS;
  }
}

// ================= OBSERVER Notification ==================

interface Observer {
  notify(user: User, booking: Booking): void;
}

class EmailNotification implements Observer {
  notify(user: User, booking: Booking) {
    console.log(`Email sent to ${user.name} for booking ${booking.id}`);
  }
}

class SMSNotification implements Observer {
  notify(user: User, booking: Booking) {
    console.log(`SMS sent to ${user.name} for booking ${booking.id}`);
  }
}

class NotificationService {
  private static instance: NotificationService;
  private observers: Observer[] = [];

  private constructor() {}

  static getInstance(): NotificationService {
    if (!this.instance) this.instance = new NotificationService();
    return this.instance;
  }

  addObserver(observer: Observer) {
    this.observers.push(observer);
  }

  notify(user: User, booking: Booking) {
    this.observers.forEach((o) => o.notify(user, booking));
  }
}

// ================= BOOKING =================

class Booking {
  constructor(
    public id: string,
    public user: User,
    public show: Show,
    public seats: string[],
    public amount: number,
    public status: BookingStatus,
  ) {}
}

// ================= BOOKMYSHOW FACADE =================

class BookMyShow {
  private static instance: BookMyShow;

  private users = new Map<string, User>();
  private cities = new Map<string, City>();
  private movies = new Map<string, Movie>();
  private theatres = new Map<string, Theatre>();
  private shows = new Map<string, Show>();
  private bookings = new Map<string, Booking>();

  private cityMovies = new Map<string, Set<string>>();
  private cityMovieShows = new Map<string, Map<string, Show[]>>();

  private constructor() {}

  static getInstance(): BookMyShow {
    if (!this.instance) this.instance = new BookMyShow();
    return this.instance;
  }

  // ---------- ADMIN ----------

  addCity(city: City) {
    this.cities.set(city.id, city);
  }

  addMovie(movie: Movie) {
    this.movies.set(movie.id, movie);
  }

  addTheatre(theatre: Theatre) {
    this.theatres.set(theatre.id, theatre);
  }

  addShow(show: Show) {
    this.shows.set(show.id, show);

    const cityId = show.theatre.city.id;
    const movieId = show.movie.id;

    if (!this.cityMovies.has(cityId)) this.cityMovies.set(cityId, new Set());

    this.cityMovies.get(cityId)!.add(movieId);

    if (!this.cityMovieShows.has(cityId)) this.cityMovieShows.set(cityId, new Map());

    const movieMap = this.cityMovieShows.get(cityId)!;

    if (!movieMap.has(movieId)) movieMap.set(movieId, []);

    movieMap.get(movieId)!.push(show);
  }

  // ---------- USER ----------

  registerUser(id: string, name: string): User {
    const user = new User(id, name);
    this.users.set(id, user);
    return user;
  }

  getMoviesByCity(cityId: string): Movie[] {
    const movieIds = this.cityMovies.get(cityId);
    if (!movieIds) return [];
    return Array.from(movieIds).map((id) => this.movies.get(id)!);
  }

  getShows(cityId: string, movieId: string): Show[] {
    return this.cityMovieShows.get(cityId)?.get(movieId) || [];
  }

  // ---------- BOOKING ----------

  bookTicket(userId: string, showId: string, seats: string[], payment: PaymentStrategy): Booking {
    const user = this.users.get(userId)!;
    const show = this.shows.get(showId)!;

    show.lockSeats(seats, userId);

    try {
      let total = 0;
      for (let seat of seats) total += show.getSeatPrice(seat);

      const paymentStatus = payment.pay(total);

      if (paymentStatus !== PaymentStatus.SUCCESS) {
        show.releaseSeats(seats, userId);
        throw new Error("Payment failed");
      }

      show.confirmSeats(seats);

      const booking = new Booking("B-" + Date.now(), user, show, seats, total, BookingStatus.CONFIRMED);

      this.bookings.set(booking.id, booking);
      user.bookings.push(booking);

      NotificationService.getInstance().notify(user, booking);

      return booking;
    } catch (err) {
      show.releaseSeats(seats, userId);
      throw err;
    }
  }
}

// ================= MAIN =================

function main() {
  const bms = BookMyShow.getInstance();
  const notificationService = NotificationService.getInstance();

  notificationService.addObserver(new EmailNotification());
  notificationService.addObserver(new SMSNotification());

  const city = new City("C1", "Delhi");
  bms.addCity(city);

  const movie = new Movie("M1", "Avengers", true);
  bms.addMovie(movie);

  const seats = [new Seat("A1", SeatCategory.SILVER), new Seat("A2", SeatCategory.GOLD)];

  const screen = new Screen("SC1", "IMAX Screen", ScreenType.IMAX, seats);

  const theatre = new Theatre("T1", "PVR", city);
  theatre.addScreen(screen);
  bms.addTheatre(theatre);

  const show = new Show("S1", movie, theatre, screen, new Date(), new StandardPricingStrategy());

  bms.addShow(show);

  const user = bms.registerUser("U1", "Rahul");

  const booking = bms.bookTicket(user.id, show.id, ["A2"], new UPIPayment());

  console.log("Booking Confirmed:", booking.id);
}

main();
