# DbAccess

Lớp bọc mỏng cho **Qt SQL** (`QSqlDatabase` / `QSqlQuery`), giúp truy cập database gọn hơn và an toàn hơn trong ứng dụng Qt, kể cả ứng dụng đa luồng.

```cpp
auto db = Database::acquire("main");
auto users = db.query("SELECT id, name FROM users WHERE age > :age")
               .bind("age", 18)
               .list(mapUser);

if (!users.ok())
    qWarning() << users.error().toString();
```

- **Không ORM**: bạn tự viết SQL, thư viện chỉ lo phần lặp đi lặp lại.
- **Lỗi không thể "quên kiểm tra"**: mọi thao tác trả về `SqlResult`, kèm lý do lỗi và câu SQL gây lỗi.
- **Không bao giờ throw exception.**
- **An toàn đa luồng**: mỗi thread tự có connection riêng, tự dọn khi thread kết thúc.
- **Transaction tự rollback** nếu bạn không commit (return sớm, lỗi giữa chừng...).
- **Luôn dùng prepared statement**: không có SQL injection qua tham số.
- Hỗ trợ mọi driver của Qt SQL: PostgreSQL (`QPSQL`), MySQL/MariaDB (`QMYSQL`), SQLite (`QSQLITE`), ODBC...

---

## Mục lục

- [Yêu cầu](#yêu-cầu)
- [Cài đặt](#cài-đặt)
- [Bắt đầu nhanh](#bắt-đầu-nhanh)
- [Cấu hình kết nối](#cấu-hình-kết-nối)
- [Đăng ký database và tắt ứng dụng](#đăng-ký-database-và-tắt-ứng-dụng)
- [Lấy connection: `Database`](#lấy-connection-database)
- [Chạy query](#chạy-query)
- [Đọc dữ liệu từ một dòng: `SqlRow`](#đọc-dữ-liệu-từ-một-dòng-sqlrow)
- [Xử lý kết quả và lỗi](#xử-lý-kết-quả-và-lỗi)
- [Transaction](#transaction)
- [Đa luồng](#đa-luồng)
- [Dùng thẳng Qt khi cần](#dùng-thẳng-qt-khi-cần)
- [Hạn chế hiện tại](#hạn-chế-hiện-tại)
- [Câu hỏi thường gặp](#câu-hỏi-thường-gặp)
- [Chạy test](#chạy-test)

---

## Yêu cầu

- **Qt ≥ 6.8** (`Core`, `Sql`)
- **C++17**
- Driver SQL tương ứng phải có sẵn trong bản Qt của bạn. `QSQLITE` luôn có; `QPSQL`/`QMYSQL` cần cài thư viện client của DB (và đôi khi phải tự build plugin, nhất là `QMYSQL`).
- Ứng dụng phải có `QCoreApplication` (hoặc `QGuiApplication` / `QApplication`), vì Qt SQL yêu cầu.

## Cài đặt

Thêm repo vào project như một git submodule:

```bash
git submodule add https://github.com/dienDan10/qt-common-libs.git external/qt-common-libs
```

Trong `CMakeLists.txt` của project, chỉ thêm module cần dùng:

```cmake
add_subdirectory(external/qt-common-libs/src/dbaccess)

target_link_libraries(myapp PRIVATE QtCommon::DbAccess)
```

Trong code, include một header duy nhất:

```cpp
#include "dbaccess/dbaccess.h"
using namespace QtCommon::dbaccess;   // chỉ trong file .cpp, không đặt trong header
```

---

## Bắt đầu nhanh

```cpp
#include <QCoreApplication>
#include "dbaccess/dbaccess.h"

using namespace QtCommon::dbaccess;

struct User
{
    int id = 0;
    QString name;
    std::optional<QString> email;   // cột cho phép NULL
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // 1. Cấu hình và đăng ký database (một lần, lúc khởi động)
    DbConfig cfg;
    cfg.name = "main";
    cfg.driver = "QSQLITE";
    cfg.database = "app.db";

    if (SqlError err = DbRegistry::instance().add(std::make_shared<ThreadLocalProvider>(cfg));
        err.hasError()) {
        qFatal("Cannot register database: %s", qPrintable(err.toString()));
    }
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [] { DbRegistry::instance().closeAll(); });

    // 2. Lấy connection
    auto db = Database::acquire("main");
    if (!db.isValid()) {
        qCritical() << db.error().toString();
        return 1;
    }

    // 3. Chạy query
    db.query("CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT NOT NULL, email TEXT)").exec();
    db.query("INSERT INTO users (name, email) VALUES (:name, :email)")
        .bind("name", "An")
        .bind("email", "an@example.com")
        .exec();

    // 4. Đọc kết quả thành struct
    auto users = db.query("SELECT id, name, email FROM users ORDER BY id")
                     .list([](const SqlRow &row) {
                         return User{row.get<int>("id"), row.get<QString>("name"), row.opt<QString>("email")};
                     });

    if (!users.ok()) {
        qCritical() << users.error().toString();
        return 1;
    }
    for (const User &u : users.value())
        qInfo() << u.id << u.name << u.email.value_or("(no email)");

    return 0;
}
```

---

## Cấu hình kết nối

`DbConfig` là một struct thường, điền các trường cần thiết:

| Trường | Ý nghĩa | Mặc định |
|---|---|---|
| `name` | Tên để gọi lại database này (`Database::acquire("main")`) | `"default"` |
| `driver` | `"QPSQL"`, `"QMYSQL"`, `"QSQLITE"`, `"QODBC"`... | (bắt buộc) |
| `host` | Địa chỉ server. **Không cần với SQLite** | |
| `port` | Cổng. `-1` = cổng mặc định của driver | `-1` |
| `database` | Tên database, hoặc đường dẫn file với SQLite (`":memory:"` = DB trong RAM) | (bắt buộc) |
| `user`, `password` | Thông tin đăng nhập | |
| `connectOptions` | Tùy chọn riêng của driver, ngăn cách bằng `;` | |
| `validationQuery` | Câu dùng để kiểm tra connection còn sống. Rỗng = không kiểm tra | `"SELECT 1"` |
| `validateAfterIdle` | Connection để không lâu hơn mức này thì kiểm tra trước khi dùng | `30s` |

**PostgreSQL**

```cpp
DbConfig cfg;
cfg.name = "main";
cfg.driver = "QPSQL";
cfg.host = "localhost";
cfg.port = 5432;
cfg.database = "gis";
cfg.user = "app";
cfg.password = qEnvironmentVariable("DB_PASSWORD");   // đừng hard-code password
cfg.connectOptions = "connect_timeout=5";
```

**MySQL / MariaDB**

```cpp
DbConfig cfg;
cfg.name = "main";
cfg.driver = "QMYSQL";
cfg.host = "127.0.0.1";
cfg.database = "shop";
cfg.user = "app";
cfg.password = qEnvironmentVariable("DB_PASSWORD");
cfg.connectOptions = "MYSQL_OPT_CONNECT_TIMEOUT=5";
```

**SQLite**

```cpp
DbConfig cfg;
cfg.name = "local";
cfg.driver = "QSQLITE";
cfg.database = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/app.db";
cfg.connectOptions = "QSQLITE_BUSY_TIMEOUT=5000";   // nhiều thread cùng ghi: chờ tối đa 5s thay vì báo "database is locked"
```

> Với SQLite dùng đa luồng, nên bật WAL một lần sau khi mở DB: `db.query("PRAGMA journal_mode=WAL").exec();`

**Kiểm tra connection còn sống.** Khi DB server khởi động lại, connection cũ "chết" nhưng phía client không biết. Thư viện tự xử lý như sau: trước khi đưa connection ra, nếu connection đã để không lâu hơn `validateAfterIdle`, nó chạy `validationQuery`; lỗi thì đóng và mở lại. Connection đang được dùng liên tục thì không kiểm tra (để không tốn thêm một lượt gửi/nhận mỗi lần).

```cpp
cfg.validateAfterIdle = std::chrono::seconds(10);   // kiểm tra thường xuyên hơn
cfg.validationQuery = "SELECT 1 FROM DUAL";          // Oracle cần cú pháp riêng
cfg.validationQuery.clear();                         // tắt hẳn
```

---

## Đăng ký database và tắt ứng dụng

Mỗi database được đăng ký **một lần** lúc khởi động, kèm một *provider* (chiến lược quản lý connection). Hiện tại có `ThreadLocalProvider`: mỗi thread một connection riêng.

```cpp
if (SqlError err = DbRegistry::instance().add(std::make_shared<ThreadLocalProvider>(cfg)); err.hasError()) {
    // Config sai được phát hiện NGAY lúc này, không đợi tới query đầu tiên:
    //   InvalidConfig: thiếu driver / database / host, trùng tên đã đăng ký...
    //   NoDriver:      driver chưa cài; thông báo liệt kê các driver đang có
    qFatal("%s", qPrintable(err.toString()));
}
```

Nhiều database cùng lúc: đăng ký nhiều lần với `name` khác nhau.

```cpp
pgConfig.name = "main";
sqliteConfig.name = "cache";
DbRegistry::instance().add(std::make_shared<ThreadLocalProvider>(pgConfig));
DbRegistry::instance().add(std::make_shared<ThreadLocalProvider>(sqliteConfig));

auto mainDb  = Database::acquire("main");
auto cacheDb = Database::acquire("cache");
```

**Khi tắt ứng dụng**, gọi `closeAll()` để đóng connection của thread chính (connection của các thread khác tự đóng khi thread đó kết thúc):

```cpp
QObject::connect(&app, &QCoreApplication::aboutToQuit, [] { DbRegistry::instance().closeAll(); });
```

---

## Lấy connection: `Database`

```cpp
auto db = Database::acquire("main");          // tên đã đăng ký
auto db = Database::acquire();                // = Database::acquire("default")
auto db = Database::acquire("main", std::chrono::seconds(2));   // timeout chờ connection (dành cho pool sau này)

if (!db.isValid()) {
    // Không có database tên đó, không mở được connection, provider đã đóng...
    qWarning() << db.error().toString();
}
```

- `Database` giữ connection trong suốt thời gian nó tồn tại và **tự trả** khi hết scope.
- Nếu `acquire` lỗi, bạn vẫn có thể gọi `db.query(...)`, nhưng mọi kết quả sẽ mang đúng lỗi đó. Vì vậy có thể kiểm tra một lần ở cuối thay vì sau từng bước.

> **Phải tách thành hai dòng.** Dòng sau **không compile**, và đó là chủ đích:
> ```cpp
> auto q = Database::acquire("main").query("SELECT 1");   // lỗi: 'query' is deleted
> ```
> `Database` tạm sẽ bị hủy ngay cuối câu lệnh, để lại `q` trỏ vào connection đã trả. Thư viện chặn điều này lúc compile. Viết:
> ```cpp
> auto db = Database::acquire("main");
> auto q = db.query("SELECT 1");
> ```

---

## Chạy query

`db.query(sql)` tạo một query. Gắn tham số bằng `bind`, rồi kết thúc bằng **một** trong bốn hàm chạy:

| Hàm | Dùng khi | Trả về | Không có dòng nào |
|---|---|---|---|
| `exec()` | `INSERT`, `UPDATE`, `DELETE`, `CREATE`... | `SqlResult<ExecInfo>` | — |
| `scalar<T>()` | Một giá trị: `COUNT(*)`, `RETURNING id`... | `SqlResult<T>` | **Lỗi** `NoRow` |
| `first(mapper)` | Tìm một bản ghi, có thể không có | `SqlResult<std::optional<R>>` | `ok`, giá trị `nullopt` |
| `list(mapper)` | Lấy nhiều bản ghi | `SqlResult<QList<R>>` | `ok`, list rỗng |

### Tham số: `bind`

Luôn dùng tham số có tên (`:ten`). **Không bao giờ nối chuỗi giá trị vào SQL.**

```cpp
db.query("SELECT * FROM users WHERE name = :name AND age >= :age")
    .bind("name", name)     // "name" hay ":name" đều được
    .bind(":age", 18);

// Gắn nhiều tham số một lúc
db.query("INSERT INTO users (name, age) VALUES (:name, :age)")
    .bind(QVariantMap{{"name", "An"}, {"age", 20}})
    .exec();

// Giá trị NULL: truyền QVariant rỗng
db.query("UPDATE users SET email = :email WHERE id = :id")
    .bind("email", QVariant())
    .bind("id", 5)
    .exec();
```

> Tên cột, tên bảng, `ORDER BY ...` **không bind được** (đây là giới hạn của SQL, không phải của thư viện). Nếu phải ghép chúng từ dữ liệu người dùng, hãy kiểm tra theo một danh sách cho phép.

### `exec()`: INSERT / UPDATE / DELETE

```cpp
auto r = db.query("UPDATE users SET active = 0 WHERE last_login < :d")
             .bind("d", QDate::currentDate().addYears(-1))
             .exec();

if (r.ok())
    qInfo() << r.value().rowsAffected << "users deactivated";
```

`ExecInfo` có hai trường:
- `rowsAffected`: số dòng bị ảnh hưởng (`-1` nếu driver không xác định được).
- `lastInsertId`: id vừa được tạo (`QVariant`). **Phụ thuộc driver**: dùng được với SQLite, MySQL; **với PostgreSQL thường không có**, hãy dùng `RETURNING`:

```cpp
// PostgreSQL: lấy id vừa tạo
auto id = db.query("INSERT INTO users (name) VALUES (:name) RETURNING id")
              .bind("name", "An")
              .scalar<int>();
```

### `scalar<T>()`: một giá trị

Lấy cột đầu tiên của dòng đầu tiên.

```cpp
auto count = db.query("SELECT COUNT(*) FROM users").scalar<int>();
auto name  = db.query("SELECT name FROM users WHERE id = :id").bind("id", 1).scalar<QString>();

if (!name.ok() && name.error().type == SqlError::Type::NoRow)
    qInfo() << "user not found";
```

- Không có dòng nào → lỗi `NoRow`.
- Giá trị là `NULL` (ví dụ `SELECT MAX(x)` trên bảng rỗng) → trả về `T{}` (`0`, chuỗi rỗng...). Nếu cần phân biệt `NULL` với `0`, dùng `first`:
  ```cpp
  auto maxAge = db.query("SELECT MAX(age) FROM users").first([](const SqlRow &r) { return r.opt<int>(0); });
  // maxAge.value() là std::optional<std::optional<int>>: ngoài = có dòng, trong = có giá trị
  ```

### `first(mapper)`: một bản ghi, có thể không có

```cpp
auto found = db.query("SELECT * FROM users WHERE email = :e").bind("e", email).first(mapUser);

if (!found.ok()) {
    // lỗi thật: SQL sai, mất kết nối...
} else if (!found.value()) {
    // chạy thành công nhưng không có bản ghi nào
} else {
    const User &u = *found.value();
}
```

### `list(mapper)`: nhiều bản ghi

```cpp
auto users = db.query("SELECT * FROM users WHERE age > :age ORDER BY name").bind("age", 18).list(mapUser);

if (users.ok())
    for (const User &u : users.value()) { /* ... */ }
```

### Mapper

Mapper là bất kỳ hàm/lambda nào nhận `const SqlRow&` và trả về một giá trị. Kiểu kết quả **tự suy ra** từ mapper, không cần ghi `<User>`.

```cpp
// Hàm thường: viết một lần, dùng lại ở nhiều query
User mapUser(const SqlRow &r)
{
    return User{r.get<int>("id"), r.get<QString>("name"), r.opt<QString>("email")};
}

auto all   = db.query("SELECT * FROM users").list(mapUser);                       // QList<User>
auto names = db.query("SELECT name FROM users").list([](const SqlRow &r) {      // QList<QString>
    return r.get<QString>(0);
});
```

### Chạy lại một query

Các hàm chạy không thay đổi query, nên có thể chạy lại nhiều lần:

```cpp
const auto countUsers = db.query("SELECT COUNT(*) FROM users");
int before = countUsers.scalar<int>().valueOr(0);
// ... thêm dữ liệu ...
int after = countUsers.scalar<int>().valueOr(0);
```

---

## Đọc dữ liệu từ một dòng: `SqlRow`

| Hàm | Ý nghĩa |
|---|---|
| `get<T>("col")` / `get<T>(index)` | Đọc và đổi kiểu. `NULL` hoặc không đổi được kiểu → `T{}` |
| `opt<T>("col")` / `opt<T>(index)` | Như `get`, nhưng `NULL` → `std::nullopt` |
| `isNull("col")` / `isNull(index)` | Cột có phải `NULL` không |
| `record()` | Bản sao cả dòng (`QSqlRecord`): tên cột + giá trị |

```cpp
auto mapProduct = [](const SqlRow &r) {
    Product p;
    p.id        = r.get<qint64>("id");
    p.name      = r.get<QString>("name");
    p.price     = r.get<double>("price");
    p.createdAt = r.get<QDateTime>("created_at");
    p.discount  = r.opt<double>("discount");   // có thể NULL
    return p;
};
```

**Chuyển một dòng bất kỳ thành JSON** (không cần định nghĩa struct, tiện cho REST API):

```cpp
QJsonObject rowToJson(const SqlRow &row)
{
    const QSqlRecord rec = row.record();
    QJsonObject obj;
    for (int i = 0; i < rec.count(); ++i)
        obj.insert(rec.fieldName(i), QJsonValue::fromVariant(rec.value(i)));
    return obj;
}

auto rows = db.query("SELECT * FROM products WHERE price < :p").bind("p", 100).list(rowToJson);
```

> `SqlRow` chỉ hợp lệ **bên trong mapper**. Không lưu nó lại (thư viện không cho copy). Cần giữ dữ liệu thì lưu giá trị đã đọc, hoặc `record()`.

---

## Xử lý kết quả và lỗi

### `SqlResult<T>`

| Hàm | Ý nghĩa |
|---|---|
| `ok()` / `if (r)` | Chạy thành công hay không |
| `value()` | Lấy giá trị. **Chỉ gọi sau khi đã kiểm tra `ok()`** |
| `valueOr(def)` | Lấy giá trị, hoặc `def` nếu lỗi |
| `error()` | `SqlError` mô tả lỗi |

```cpp
auto r = db.query("SELECT COUNT(*) FROM orders").scalar<int>();

// Cách 1: kiểm tra rõ ràng
if (!r.ok()) {
    qWarning() << r.error().toString();
    return;
}
int n = r.value();

// Cách 2: có giá trị dự phòng
int n = db.query("SELECT COUNT(*) FROM orders").scalar<int>().valueOr(0);
```

> **Gọi `value()` khi đang lỗi là lỗi lập trình.** Bản Debug: dừng chương trình ngay tại chỗ (`Q_ASSERT`) để bạn phát hiện sớm. Bản Release: trả về `T{}` và chạy tiếp.

> **Cẩn thận với `SqlResult<bool>`**: `if (r)` nghĩa là *chạy thành công*, **không phải** giá trị là `true`. Đọc giá trị bằng `r.value()`.

### `SqlError`

| Trường | Ý nghĩa |
|---|---|
| `type` | Loại lỗi (bảng dưới) |
| `message` | Thông báo lỗi |
| `nativeCode` | Mã lỗi gốc của DB, ví dụ PostgreSQL `23505` = trùng khóa unique |
| `sql` | Câu SQL gây lỗi (nếu có) |
| `hasError()` | Có lỗi hay không |
| `toString()` | Chuỗi đầy đủ để log: `message [nativeCode] \| sql: ...` |

| `SqlError::Type` | Khi nào |
|---|---|
| `InvalidConfig` | Config sai, hoặc `acquire` một tên chưa đăng ký |
| `NoDriver` | Driver chưa được cài |
| `Connection` | Không mở được connection, provider đã đóng |
| `Statement` | SQL sai cú pháp, sai tên bảng/cột, vi phạm ràng buộc... |
| `Transaction` | BEGIN / COMMIT thất bại, transaction lồng nhau |
| `NoRow` | `scalar()` không có dòng nào |
| `Timeout` | Chờ connection quá hạn (dành cho pool sau này) |
| `Unknown` | Các lỗi còn lại |

**Ví dụ: chuyển lỗi thành mã HTTP** trong một handler:

```cpp
int httpStatus(const SqlError &e)
{
    switch (e.type) {
    case SqlError::Type::NoRow:      return 404;
    case SqlError::Type::Connection:
    case SqlError::Type::Timeout:    return 503;
    default:                         return 500;
    }
}

// Lỗi trùng dữ liệu (PostgreSQL)
if (!r.ok() && r.error().nativeCode == "23505")
    return ctx->error(409, "email already exists");
```

**Lỗi được truyền thẳng qua.** Nếu `Database::acquire` thất bại, mọi query trên nó trả về chính lỗi đó, nên bạn có thể kiểm tra một lần ở cuối:

```cpp
auto db = Database::acquire("main");                       // giả sử lỗi Connection
auto r = db.query("SELECT ...").list(mapUser);             // không chạy SQL, mang lỗi Connection
if (!r.ok()) return httpStatus(r.error());                 // 503
```

---

## Transaction

```cpp
SqlResult<void> transfer(Database &db, int from, int to, double amount)
{
    auto tx = db.transaction();                 // BEGIN
    if (!tx.isActive())
        return tx.error();

    auto r1 = db.query("UPDATE accounts SET balance = balance - :a WHERE id = :id")
                  .bind("a", amount).bind("id", from).exec();
    if (!r1.ok())
        return r1.error();                      // return sớm -> tự ROLLBACK

    auto r2 = db.query("UPDATE accounts SET balance = balance + :a WHERE id = :id")
                  .bind("a", amount).bind("id", to).exec();
    if (!r2.ok())
        return r2.error();                      // tự ROLLBACK

    return tx.commit();                         // COMMIT, trả về lỗi nếu commit thất bại
}
```

| Hàm | Ý nghĩa |
|---|---|
| `db.transaction()` | Bắt đầu transaction |
| `isActive()` / `error()` | BEGIN có thành công không, lỗi gì |
| `commit()` | Lưu thay đổi. Lỗi thì tự rollback để connection sạch, và trả về lỗi |
| `rollback()` | Hủy thay đổi. Gọi thừa cũng không sao |
| (hết scope) | **Tự ROLLBACK** nếu chưa commit |

Lưu ý:
- **Không lồng transaction được.** Tạo transaction thứ hai trên cùng connection khi transaction đầu chưa xong sẽ thất bại (`isActive() == false`, lỗi `Transaction`).
- Gọi `commit()` hai lần → lần thứ hai trả lỗi.
- Driver phải hỗ trợ transaction (PostgreSQL, SQLite, MySQL InnoDB đều có).

---

## Đa luồng

**Quy tắc ngắn gọn:**
1. Gọi `Database::acquire()` **ngay trong thread sẽ dùng nó**.
2. **Không truyền** `Database`, query hay transaction sang thread khác.
3. Không giữ chúng lâu hơn cần thiết. Lấy, dùng, để tự trả khi hết scope.

```cpp
QThreadPool::globalInstance()->start([] {
    auto db = Database::acquire("main");        // connection của RIÊNG thread này
    auto r = db.query("SELECT ...").list(map);
    // ...
});                                             // hết scope: tự trả
```

**Mỗi thread có một connection riêng** cho mỗi database, được mở lần đầu khi thread đó gọi `acquire`, dùng lại cho các lần sau, và **tự đóng khi thread kết thúc**. Không cần lock gì cả.

**Dùng với httpserver (`QtCommon::HttpServer`):** chỉ truy cập DB trong **worker route** (mặc định), **không** dùng trong route `runSync()` (route đó chạy trên thread của server; query sẽ chặn toàn bộ server).

**Gọi `acquire` lồng nhau trong cùng thread → cùng một connection.** Điều này thường là điều bạn muốn (hàm con chạy chung transaction với hàm cha):

```cpp
auto outer = Database::acquire("main");
auto tx = outer.transaction();
saveOrder(order);        // bên trong lại gọi Database::acquire("main") -> CÙNG connection, CÙNG transaction
tx.commit();
```

Nếu cần một connection **độc lập** tới cùng database trong cùng thread (ví dụ ghi log audit phải được lưu kể cả khi transaction chính bị rollback), đăng ký thêm một tên:

```cpp
cfg.name = "main_audit";   // cùng DB, tên khác -> connection khác
DbRegistry::instance().add(std::make_shared<ThreadLocalProvider>(cfg));

auto audit = Database::acquire("main_audit");
```

**Số connection = số thread có dùng DB.** Với `QThreadPool` có 8 thread, tối đa 8 connection mỗi database.

> `QThreadPool::waitForDone()` không chỉ chờ task xong, nó còn **cho các thread kết thúc**, kéo theo đóng hết connection của chúng. Lần chạy sau phải mở lại connection.

---

## Dùng thẳng Qt khi cần

Khi cần một tính năng thư viện chưa bọc (ví dụ `QSqlTableModel`, `execBatch`), lấy `QSqlDatabase` gốc:

```cpp
auto db = Database::acquire("main");
QSqlDatabase raw = db.lease().database();

{
    QSqlQuery q(raw);
    q.prepare("INSERT INTO logs (msg) VALUES (?)");
    q.addBindValue(QVariantList{"a", "b", "c"});
    q.execBatch();
} // q phải bị hủy trước db
```

Khi đó bạn tự chịu trách nhiệm: **không giữ `raw` hay `QSqlQuery` lâu hơn `db`**, và không dùng chúng ở thread khác.

---

## Hạn chế hiện tại

- **Chỉ có `ThreadLocalProvider`.** Số connection gắn chặt với số thread. Chưa có connection pool giới hạn số connection độc lập với số thread (dự kiến: `PooledProvider`).
- **Chỉ đồng bộ (blocking).** Query chặn thread đang gọi cho tới khi DB trả lời. Chưa có API trả `QFuture` (dự kiến: `DbExecutor`). Đừng chạy query trên thread giao diện hoặc thread của server.
- **Chỉ tham số có tên (`:name`).** Chưa hỗ trợ tham số vị trí (`?`) và chạy hàng loạt (`execBatch`) qua API của thư viện. Dùng [Qt thuần](#dùng-thẳng-qt-khi-cần) nếu cần.
- **Chưa có query builder** (dựng `WHERE`/`ORDER BY`/`LIMIT` động, insert/update từ `QVariantMap`, phân trang kèm tổng số dòng). Hiện phải tự viết SQL.
- **Không hỗ trợ transaction lồng nhau / SAVEPOINT.**
- **Không che khác biệt giữa các DB**: `RETURNING`, upsert, kiểu JSON/boolean, `LIMIT` vs `TOP`... bạn tự viết đúng cú pháp của DB mình dùng.
- **`lastInsertId` phụ thuộc driver**; với PostgreSQL dùng `RETURNING`.
- **`scalar()` với giá trị `NULL` trả về `T{}`**, không phân biệt được với `0`. Dùng `first()` + `opt<T>()` nếu cần.
- **`get<T>()` với tên cột sai** trả về `T{}` kèm một cảnh báo của Qt, không trả lỗi. Đổi kiểu thất bại (ví dụ chuỗi `"abc"` sang `int`) cũng trả `T{}` mà không báo.
- **Không tự thử lại khi mất kết nối giữa chừng.** Connection chết trong lúc đang dùng → query đó trả lỗi `Connection`/`Statement`; lần `acquire` sau (nếu connection đã để không đủ lâu) sẽ tự mở lại. Việc thử lại câu query là việc của bạn.
- **`closeAll()` chỉ đóng ngay connection của thread gọi nó.** Thread khác sẽ đóng connection của mình ở lần `acquire` kế tiếp (và nhận lỗi) hoặc khi thread kết thúc.
- **Chưa hỗ trợ LISTEN/NOTIFY** của PostgreSQL.
- Thread tạo bằng `std::thread` (không phải `QThread`) chỉ được dọn connection khi thread kết thúc trong lúc ứng dụng còn chạy; nên dùng `QThread` / `QThreadPool`.

---

## Câu hỏi thường gặp

**`error: call to deleted member function 'query'`**
Bạn đang gọi `query()` / `transaction()` trên `Database` tạm. Tách thành hai dòng: `auto db = Database::acquire(...); db.query(...)`. Xem [Lấy connection](#lấy-connection-database).

**`QSqlDatabasePrivate::removeDatabase: connection '...' is still in use`**
Bạn đang giữ một `QSqlDatabase` hoặc `QSqlQuery` (lấy qua `lease().database()`) lâu hơn `Database`. Đặt chúng trong một khối `{ }` để chúng bị hủy trước.

**`QSqlDatabase: QMYSQL driver not loaded`** / lỗi `NoDriver` khi đăng ký
Driver chưa có trong bản Qt của bạn. Thông báo lỗi liệt kê các driver đang có. Cài thư viện client của DB, hoặc build plugin SQL tương ứng.

**SQLite báo `database is locked`**
Nhiều thread cùng ghi. Thêm `cfg.connectOptions = "QSQLITE_BUSY_TIMEOUT=5000";` và bật WAL (`PRAGMA journal_mode=WAL`).

**Query trả về rỗng nhưng tôi chắc là có dữ liệu**
Kiểm tra `ok()` trước khi đọc giá trị: một kết quả lỗi đọc bằng `valueOr` sẽ trông như rỗng. In `error().toString()`: nó kèm luôn câu SQL đã chạy.

**Dữ liệu ghi xong không thấy ở thread khác / bị mất**
Có thể bạn quên `commit()`: transaction hết scope sẽ tự rollback.

**Tôi muốn hai thao tác chạy trong cùng một transaction nhưng ở hai hàm khác nhau**
Trong cùng một thread, chỉ cần cả hai hàm cùng `acquire` cùng tên database: chúng dùng chung connection nên dùng chung transaction đang mở.

---

## Chạy test

```bash
cmake -S . -B build
cmake --build build
cd build && ctest --output-on-failure
```

Test dùng SQLite (không cần cài DB server), gồm cả test đa luồng `tst_dbconcurrency`.